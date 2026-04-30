#include <time.h>

#include "airoot.h"
#include "ai2.h"
#include "aileaf.h"
#include "attacks.h"
#include "bitboard.h"
#include "logging.h"
#include "moveordering.h"
#include "quadboard.h"
#include "tt.h"
#include "umpire.h"

//
// Ask an AI agent to make a move.
//
void aiMove(const board* const current, board* const next, const board* const loopDetectPtr, const int turnNumber) {
    const time_t startTime = time(NULL);
    
    nodesCalculated = 0;
    analysisMove bestmove;

    // Clear killer moves and history at the start of each search
    // (optional - you could keep them between moves for learning)
    clearKillers();
    // clearHistory(); // Uncomment to reset history each move

    ttInit();
    ttNewSearch();
    
    // Iterative deepening from depth 1 up to the target. Earlier iterations
    // populate the TT and killer tables so deeper iterations get strong
    // move ordering, more than paying back the cost of the shallow searches.
    const depthType maxDepth = 6;
    scoreType score = 0;
    for (depthType d = 1; d <= maxDepth; d++) {
        score = getBestMove(&bestmove, loopDetectPtr, current, current->whosTurn, d, 0, -9999, 9999);
    }
    
    print("\n");
    makeMove(current, next, &bestmove);
    const time_t finishTime = time(NULL);
    const double timetaken = difftime(finishTime, startTime);
    
    print("===== ai move for %s\n", current->whosTurn ? "BLACK" : "WHITE");
    print("Move chosen: ");
    byte mover = trailingBit_Bitboard(bestmove.from);
    printPieceUnicode(getType(current->quad,mover), current->whosTurn, UNICODESET_SOLID);
    printResetColors();
    print(" ");
    bitboard taken = getAllPieces(current->quad) & bestmove.to;
    if (taken) {
        byte takenOffset = trailingBit_Bitboard(bestmove.to);
        print("x ");
        printPieceUnicode(getType(current->quad,takenOffset), opponentOf(current->whosTurn), UNICODESET_SOLID);
        printResetColors();
        print(" ");
    }
    
    printMove(bestmove);
    if (isKingChecked(next->quad, next->whosTurn)) {
        print(" >>> CHECK <<<");
    }
    print(" (score: %d, nodes: %d)\n", (int)score, (int)nodesCalculated);
    print("Ai Move Time Taken: %f, processing speed %f\n", timetaken, nodesCalculated / timetaken);
    printQBUnicode(next->quad); 
}
