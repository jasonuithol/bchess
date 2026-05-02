// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//
// Now with ALPHA-BETA PRUNING and MOVE ORDERING for massive speedup!
//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "aiStrength"
//

#include <string.h>
#include <time.h>

#include "ai2.h"
#include "aileaf.h"
#include "bitboard.h"
#include "logging.h"
#include "moveordering.h"
#include "quadboard.h"
#include "tt.h"
#include "umpire.h"

volatile int searchAborted = 0;
static long searchDeadlineMs = 0;  // 0 disables the deadline.

void setSearchDeadlineMs(long absoluteMs) {
    searchDeadlineMs = absoluteMs;
    searchAborted = 0;
}

void clearSearchDeadline(void) {
    searchDeadlineMs = 0;
    searchAborted = 0;
}

void requestSearchAbort(void) {
    searchAborted = 1;
}

// Forward-declared from uci.c. Drains stdin for "stop" / "ponderhit"
// so the search responds to them mid-iteration instead of only at the
// iteration boundary. Always linked together with this file.
extern void pollSearchInput(void);

// Check whether the configured deadline has passed. Gated by a counter
// because clock_gettime, even via vDSO, would be wasteful at every node
// visit of a million-nps search. We piggy-back the rate-limited tick
// to also poll stdin so the GUI's "stop" gets noticed promptly even
// when no deadline is armed (e.g. during pondering).
static int isPastDeadline(void) {
    if (searchAborted) return 1;

    static unsigned int callCounter = 0;
    if ((++callCounter & 0xFFF) != 0) return 0;

    pollSearchInput();
    if (searchAborted) return 1;

    if (searchDeadlineMs <= 0) return 0;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    const long nowMs = now.tv_sec * 1000L + now.tv_nsec / 1000000L;
    if (nowMs >= searchDeadlineMs) {
        searchAborted = 1;
        return 1;
    }
    return 0;
}

scoreType getBestMove(analysisMove* const bestMove, const board* const loopDetect, board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth, scoreType alpha, scoreType beta, const byte nullAllowed) {

    // Bail before doing any work if the deadline has already fired.
    // The returned score is meaningless; the root caller will discard
    // the entire iteration when it sees searchAborted set.
    if (isPastDeadline()) {
        return 0;
    }

    // Capture original window for TT bound classification at the end.
    const scoreType alphaOrig = alpha;
    const scoreType betaOrig  = beta;

    // Remaining depth (used for TT validity comparison).
    const depthType depthRemaining = aiStrength - depth;

    // Probe TT. Even when the score isn't directly usable, the stored
    // best move seeds move ordering. b->hash is incrementally maintained
    // by applyMove/revertMove so this is a single field read, not a
    // 64-square scan.
    const uint64_t hash = b->hash;
#ifdef HASH_PARANOIA
    {
        const uint64_t fresh = computeZobristHash(b);
        if (fresh != hash) {
            error("HASH MISMATCH at depth=%d: incr=%016lx fresh=%016lx\n",
                  (int)depth, (unsigned long)hash, (unsigned long)fresh);
        }
    }
#endif
    const ttResult tte = ttProbe(hash, depthRemaining, alpha, beta);
    if (tte.usable && depth > 0) {
        // Non-root cut: caller doesn't need bestMove populated.
        return tte.score;
    }

    // ----------------------------------------------------------------
    // Null-move pruning.
    //
    // Premise: if I "pass" my turn and the opponent's best response
    // STILL leaves the position outside the alpha-beta window in my
    // favour, then my actual best move (which is at least as good as
    // passing) must also leave the window — prune without searching.
    //
    // Guards:
    //   - nullAllowed: prevents two passes in a row (would expand the
    //     tree without paying for itself).
    //   - depth > 0: at the root we need a concrete bestMove, not just
    //     a score; the regular search must run.
    //   - depthRemaining >= 3: with reduction R=2, a smaller remaining
    //     depth degenerates to a leaf eval and the pruning isn't worth
    //     the call overhead.
    //   - !isKingChecked: passing while in check is illegal — the king
    //     is left attacked. We can't legally null-move here.
    //   - Non-pawn material present: classic zugzwang guard. In KP
    //     endgames every move can lose, so "passing is no worse than
    //     moving" is a false premise. Skip null-move when the side to
    //     move has only king + pawns.
    // ----------------------------------------------------------------
    if (nullAllowed
        && depth > 0
        && depthRemaining >= 3
        && !isKingChecked(b->quad, b->whosTurn)) {

        const bitboard nonPawnMaterial =
              getPieces(b->quad, ROOK   | b->whosTurn)
            | getPieces(b->quad, KNIGHT | b->whosTurn)
            | getPieces(b->quad, BISHOP | b->whosTurn)
            | getPieces(b->quad, QUEEN  | b->whosTurn);

        if (nonPawnMaterial) {
            const depthType R = 2;

            // Apply null move. Save state to restore exactly. The ep
            // opportunity expires after a pass (it's a one-ply window).
            // Castling rights for the new mover need recomputing because
            // the recursive call's move-gen reads them. Hash needs the
            // ep-key swap and side-toggle XORed in so the recursive
            // TT probe sees the correct key. piecesMoved is unchanged
            // (no piece moved) so no zobCastle update.
            const bitboard prevEp             = b->enPassantTarget;
            const byte     prevWhosTurn       = b->whosTurn;
            const byte     prevCastlingRights = b->currentCastlingRights;
            const uint64_t prevHash           = b->hash;

            // Hash: XOR out OLD ep key, XOR in NEW ep key (no-ep slot 8),
            // toggle side.
            if (prevEp) {
                const offset epFile = trailingBit_Bitboard(prevEp) % 8;
                b->hash ^= zobEnPassant[epFile];
            } else {
                b->hash ^= zobEnPassant[8];
            }
            b->hash ^= zobEnPassant[8];
            b->hash ^= zobSide;

            b->enPassantTarget = 0;
            b->whosTurn       ^= 1;
            computeCurrentCastlingRights(b);

            analysisMove dummy;
            const scoreType nullScore = getBestMove(
                &dummy, loopDetect, b,
                scoringTeam, aiStrength,
                depth + 1 + R,
                alpha, beta,
                0);  // no nested null-move

            // Revert. Hash snaps back to its saved value.
            b->enPassantTarget       = prevEp;
            b->whosTurn              = prevWhosTurn;
            b->currentCastlingRights = prevCastlingRights;
            b->hash                  = prevHash;

            if (searchAborted) return 0;

            // Cutoff direction follows fixed-perspective MAX/MIN logic.
            if (b->whosTurn == scoringTeam) {
                if (nullScore >= beta)  return beta;
            } else {
                if (nullScore <= alpha) return alpha;
            }
        }
    }

    // Start by assuming the worst for us (or the best for the opponent)
    scoreType bestScore = (b->whosTurn == scoringTeam ? -9999 : 9999);

    analysisList moveList;
    moveList.ix = 0; // MANDATORY

    if (depth < aiStrength) {
        // Do non-leaf analysis
        generateLegalMoveList(b, &moveList, 0);
    }
    else {
        // Do leaf analysis
        generateLegalMoveList(b, &moveList, 1);
    }

    // Checkmate/stalemate detection for AI. Game over decision made elsewhere.
    if (moveList.ix == 0) {

        if (depth == 0) {
            error("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
        }
        else {
            return analyseLeafTerminal(b, scoringTeam, depth);
        }
    }

    // ========================================
    // MOVE ORDERING - Sort moves before searching
    // ========================================
    sortMoves(&moveList, &b->quad, depth, tte.bestFrom, tte.bestTo);

    bitboard bestFromHere = 0;
    bitboard bestToHere   = 0;

    const byte willRecurse = (depth < aiStrength);

    for (byte ix = 0; ix < moveList.ix; ix++) {
        //
        // Assess the move [from]->[to] on board b to depth aiStrength.
        //
        // We do this by spawning the resulting board, seeing what score
        // we get when we follow all the moves from then on, and checking
        // if any of those leaf level boards has a higher best score than
        // what we have now.
        //
        const analysisMove* move = &(moveList.items[ix]);
        analysisMove dummyMove;
        scoreType score;

        // Mutate b in place; revert before the next iteration. No board
        // copy at all; the same memory location is reused down the tree.
        UndoInfo undo;
        applyMove(b, move, &undo, 1);

        if (areEqualQB(loopDetect->quad, b->quad)) {

            // If a loop is detected, don't recurse, just score very badly
            score = (undo.movedTeam == scoringTeam) ? (-9998 + aiStrength + 1) : (9999 - aiStrength - 1);
        }
        else if (willRecurse) {
            // The recursive call will generate moves from the post-move
            // position, so it needs castling rights computed. applyMove
            // skips that compute by default — we pay for it only here.
            computeCurrentCastlingRights(b);
            score = getBestMove(&dummyMove, loopDetect, b, scoringTeam, aiStrength, depth + 1, alpha, beta, 1);
        }
        else {
            score = analyseLeafNonTerminal(b->quad, scoringTeam);
        }

        revertMove(b, move, &undo);

        // If the recursive call tripped the deadline, the score it
        // returned is uninitialised garbage. Stop iterating and let the
        // caller throw the whole iteration away.
        if (searchAborted) {
            return 0;
        }

        //
        // Update bestscore to be the best score.
        //
        // If it's our turn (MAX node), get the highest score
        // If it's their turn (MIN node), get the lowest score
        //
        if (b->whosTurn == scoringTeam) {
            // MAX node - we're trying to maximize
            if (score > bestScore) {
                bestScore = score;
                bestFromHere = move->from;
                bestToHere   = move->to;
                // Only useful at depth == 0
                memcpy((void*)bestMove, (void*)move, sizeof(analysisMove));
            }

            // Alpha-beta pruning for MAX node
            if (score > alpha) {
                alpha = score;

                // Beta cutoff: opponent won't let us get here
                if (beta <= alpha) {
                    // Store this as a killer move (if not a capture)
                    bitboard capturedPiece = getAllPieces(b->quad) & move->to;
                    if (!capturedPiece) {
                        addKiller(move->from, move->to, depth);

                        // Update history heuristic
                        offset fromSquare = trailingBit_Bitboard(move->from);
                        byte pieceType = getType(b->quad, fromSquare);
                        updateHistory(pieceType, move->to, depth);
                    }

                    break; // Prune remaining moves
                }
            }
        }
        else {
            // MIN node - opponent is trying to minimize
            if (score < bestScore) {
                bestScore = score;
                bestFromHere = move->from;
                bestToHere   = move->to;
                // Only useful at depth == 0
                memcpy((void*)bestMove, (void*)move, sizeof(analysisMove));
            }

            // Alpha-beta pruning for MIN node
            if (score < beta) {
                beta = score;

                // Alpha cutoff: we won't let opponent get here
                if (beta <= alpha) {
                    // Store this as a killer move (if not a capture)
                    bitboard capturedPiece = getAllPieces(b->quad) & move->to;
                    if (!capturedPiece) {
                        addKiller(move->from, move->to, depth);

                        // Update history heuristic
                        offset fromSquare = trailingBit_Bitboard(move->from);
                        byte pieceType = getType(b->quad, fromSquare);
                        updateHistory(pieceType, move->to, depth);
                    }

                    break; // Prune remaining moves
                }
            }
        }

    } // for ix

    // Classify the result for TT storage. With fixed-perspective scoring
    // both MAX and MIN nodes return their bestScore directly, so the
    // standard fail-low / fail-high / exact classification on
    // (bestScore vs alphaOrig/betaOrig) applies to both:
    //   bestScore >= betaOrig  => true score is at least bestScore => LOWER
    //   bestScore <= alphaOrig => true score is at most  bestScore => UPPER
    //   else                   => exact within the original window
    ttBound bound;
    if (bestScore >= betaOrig) {
        bound = TT_LOWER;
    }
    else if (bestScore <= alphaOrig) {
        bound = TT_UPPER;
    }
    else {
        bound = TT_EXACT;
    }
    // Don't pollute the TT with scores from an aborted iteration; they
    // were computed against an incomplete move list.
    if (!searchAborted) {
        ttStore(hash, depthRemaining, bestScore, bound, bestFromHere, bestToHere);
    }

    // Return our rating of the move now living in bestMove.
    return bestScore;
}
