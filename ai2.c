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

#include "ai2.h"
#include "aileaf.h"
#include "bitboard.h"
#include "logging.h"
#include "moveordering.h"
#include "quadboard.h"
#include "tt.h"
#include "umpire.h"

scoreType getBestMove(analysisMove* const bestMove, const board* const loopDetect, const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth, scoreType alpha, scoreType beta) {

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
                    : analyseLeafNonTerminal(newBoard.quad, scoringTeam);
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
    ttStore(hash, depthRemaining, bestScore, bound, bestFromHere, bestToHere);

    // Return our rating of the move now living in bestMove.
    return bestScore;
}
