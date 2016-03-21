// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//


//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "numMoves"
//
scoreType getBestMove(analysisMove* const bestMove, const board* const loopDetect, const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth) {
		
//	print("Entering getBestMove for %s at depth %u\n", b->whosTurn ? "BLACK" : "WHITE", depth);	
	
//	printQB(b->quad);
		
	// Start by assuming the worst for us (or the best for the opponent), and see if we can do better than that.
	scoreType bestScore = (b->whosTurn == scoringTeam ? -9999 : 9999);
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	if (depth < aiStrength) {
	
//		print("Generating Non-Leaf movelist\n");
		
		// Do non-leaf analysis
		generateLegalMoveList(b, &moveList, 0);			
	} 
	else {
		
//		print("Generating Leaf movelist\n");

		// Do leaf analysis
		generateLegalMoveList(b, &moveList, 1);			
	}

//	print("Generated %u moves\n", moveList.ix);

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		if (depth == 0) {
			error("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
		} 
		else {
			return analyseLeafTerminal(b, scoringTeam, depth);
		}
	}

	
	
	for (byte ix = 0; ix < moveList.ix; ix++) {
		
		
//		print("About to analyse resulting board from move %u\n", ix);
//		printQB(moveList.items[ix].resultingBoard.quad);
		
		//
		// Assess the move [from]->[to] on board b to depth numMoves-1.
		//
		// We do this by looking at the resulting board and then seeing what score
		// we get when we follow all the moves from then on down to numMoves -1
		// and see if any of those leaf level boards has a higher best score
		// than what we have now.
		//
		analysisMove* move = &(moveList.items[ix]);
		analysisMove dummyMove;
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			score = (b->whosTurn == scoringTeam) ? (-9998 + aiStrength + 1) : (9999 - aiStrength - 1);
	//		print("Detected looping move, setting score = %d\n", (int)score);
		}
		else {

			score = depth < aiStrength
					? getBestMove(&dummyMove, loopDetect, &(move->resultingBoard), scoringTeam, aiStrength, depth + 1)
					: analyseLeafNonTerminal(move->resultingBoard.quad, scoringTeam);
		}
		
		//
		// Update bestscore to be the best score.
		//
		// If it's our turn, get the highest (MAX), if it's their turn, get the lowest (MIN)
		//
		if ((b->whosTurn == scoringTeam) ? (score > bestScore) : (score < bestScore)) {
			
			bestScore = score;
			// Literally only useful at depth == 0;
			memcpy((void*)bestMove, (void*)move, sizeof(analysisMove));
		}

				
	} // for ix

//	print("Returning best score %d\n", bestScore);

	// Return our rating of the move now living in bestMove.
	return bestScore;
		
}

