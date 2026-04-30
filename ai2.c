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
scoreType getBestMove(analysisMove* const bestMove, const board* const loopDetect, const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth, scoreType alpha, scoreType beta) {

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
    sortMoves(&moveList, &b->quad, depth);

    for (byte ix = 0; ix < moveList.ix; ix++) {
        //
        // Assess the move [from]->[to] on board b to depth aiStrength.
        //
        // We do this by looking at the resulting board and then seeing what score
        // we get when we follow all the moves from then on and see if any of 
        // those leaf level boards has a higher best score than what we have now.
        //
        const analysisMove* move = &(moveList.items[ix]);
        analysisMove dummyMove;
        scoreType score;
        
        if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
            
            // If a loop is detected, don't recurse, just score very badly
            score = (b->whosTurn == scoringTeam) ? (-9998 + aiStrength + 1) : (9999 - aiStrength - 1);
        }
        else {
            score = depth < aiStrength
                    ? getBestMove(&dummyMove, loopDetect, &(move->resultingBoard), scoringTeam, aiStrength, depth + 1, alpha, beta)
                    : analyseLeafNonTerminal(move->resultingBoard.quad, scoringTeam);
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
    
    // Return our rating of the move now living in bestMove.
    return bestScore;
}
