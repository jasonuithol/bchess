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

// ----- Quiescence search ---------------------------------------------
//
// Standard quiescence triggers on captures, because in a material eval
// captures are the events that move the score. bchess's eval is pure
// mobility (myLegalMoves - theirLegalMoves), so the eval-correct
// trigger is "did the score change a lot from one ply to the next?",
// regardless of whether a capture happened. A pin or fork can swing
// mobility just as much as a queen capture in a material engine.
//
// At a regular leaf we compare the just-played child position's
// static eval to the parent's static eval. If the absolute delta is
// at or above QUIESCENCE_THRESHOLD we extend by another ply and let
// the side to move at the child search for a neutralising response.
// We cap recursion at QUIESCENCE_MAX_DEPTH so a sequence of swings
// can't run forever.
//
// Tuning these is a measurement problem; the values below are first
// guesses based on observed mobility-score ranges in real games.
//
#define QUIESCENCE_THRESHOLD 30
#define QUIESCENCE_MAX_DEPTH 4

static scoreType quiescenceSearch(
    const board* const b,
    const byte scoringTeam,
    scoreType alpha,
    scoreType beta,
    const scoreType parentEval,
    const depthType currentDepth,
    const depthType qDepthRemaining
) {

    // Static eval at this position.
    const scoreType currentEval = analyseLeafNonTerminal(b->quad, scoringTeam);

    // Stand pat: if the swing from the parent is small, or we've burned
    // the quiescence budget, or the search has been aborted, stop here.
    scoreType delta = currentEval - parentEval;
    if (delta < 0) delta = -delta;
    if (delta < QUIESCENCE_THRESHOLD || qDepthRemaining == 0 || searchAborted) {
        return currentEval;
    }

    // Tactical swing detected: explore moves to see if the side to move
    // can neutralise (or amplify) it. Note we look at *all* legal moves,
    // not just captures — the whole point of this design is that for a
    // mobility eval, non-capture responses (blockades, counter-pins,
    // king walks) are genuine neutralisers.
    analysisList moveList;
    moveList.ix = 0;
    generateLegalMoveList(b, &moveList, 1);

    if (moveList.ix == 0) {
        return analyseLeafTerminal(b, scoringTeam, currentDepth);
    }

    scoreType bestScore = (b->whosTurn == scoringTeam) ? -9999 : 9999;

    for (byte ix = 0; ix < moveList.ix; ix++) {
        const analysisMove* const move = &moveList.items[ix];
        board newBoard;
        spawnLeafBoard(b, &newBoard, move->from, move->to, move->promoteTo);

        const scoreType score = quiescenceSearch(
            &newBoard, scoringTeam,
            alpha, beta,
            currentEval, currentDepth + 1, qDepthRemaining - 1
        );

        if (searchAborted) return currentEval;

        if (b->whosTurn == scoringTeam) {
            if (score > bestScore) bestScore = score;
            if (score > alpha)     alpha = score;
        } else {
            if (score < bestScore) bestScore = score;
            if (score < beta)      beta = score;
        }
        if (beta <= alpha) break;
    }

    return bestScore;
}

// Check whether the configured deadline has passed. Gated by a counter
// because clock_gettime, even via vDSO, would be wasteful at every node
// visit of a million-nps search. Once the flag flips we stop calling
// the clock entirely.
static int isPastDeadline(void) {
    if (searchAborted) return 1;
    if (searchDeadlineMs <= 0) return 0;

    static unsigned int callCounter = 0;
    if ((++callCounter & 0xFFF) != 0) return 0;

    struct timespec now;
    clock_gettime(CLOCK_MONOTONIC, &now);
    const long nowMs = now.tv_sec * 1000L + now.tv_nsec / 1000000L;
    if (nowMs >= searchDeadlineMs) {
        searchAborted = 1;
        return 1;
    }
    return 0;
}

scoreType getBestMove(analysisMove* const bestMove, const board* const loopDetect, const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth, scoreType alpha, scoreType beta) {

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
    // best move seeds move ordering.
    const uint64_t hash = computeZobristHash(b);
    const ttResult tte = ttProbe(hash, depthRemaining, alpha, beta);
    if (tte.usable && depth > 0) {
        // Non-root cut: caller doesn't need bestMove populated.
        return tte.score;
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

    // For quiescence: when this is the level whose children get leaf-
    // evaluated, we need the static eval of *this* position so each
    // child can compute its delta against it. Computing it once here
    // is cheaper than re-computing per child.
    const scoreType staticEval = willRecurse ? 0 : analyseLeafNonTerminal(b->quad, scoringTeam);

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

        // Spawn the resulting board locally. Non-leaf descendants need
        // the castling-rights overlay; leaf-level only needs piece state.
        board newBoard;
        if (willRecurse) {
            spawnFullBoard(b, &newBoard, move->from, move->to, move->promoteTo);
        } else {
            spawnLeafBoard(b, &newBoard, move->from, move->to, move->promoteTo);
        }

        if (areEqualQB(loopDetect->quad, newBoard.quad)) {

            // If a loop is detected, don't recurse, just score very badly
            score = (b->whosTurn == scoringTeam) ? (-9998 + aiStrength + 1) : (9999 - aiStrength - 1);
        }
        else {
            score = willRecurse
                    ? getBestMove(&dummyMove, loopDetect, &newBoard, scoringTeam, aiStrength, depth + 1, alpha, beta)
                    : quiescenceSearch(&newBoard, scoringTeam, alpha, beta,
                                       staticEval, depth + 1, QUIESCENCE_MAX_DEPTH);
        }

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
