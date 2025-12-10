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
    
    // Initialize alpha-beta window to widest possible range
    scoreType alpha = -9999;
    scoreType beta = 9999;
    
    // No PV on first call
    bitboard pvFrom = 0;
    bitboard pvTo = 0;
    
    scoreType score = getBestMove(&bestmove, loopDetectPtr, current, current->whosTurn, 6, 0, alpha, beta, pvFrom, pvTo);
    
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
