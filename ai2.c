// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//

//
// "Recursively" search for the best move and set "bestMove" to point to it.
//

//
// OPPONENT TURN: ~b->whosTurn
//
scoreType level3(	const board* const loopDetect, 
					const board* const b) {

	// Start by assuming the best us, and see if the opponent can make it worse.
	scoreType bestScore = 9999;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do LEAF analysis !!! MUCH LIGHTER LOAD (No castling rights checked)
	generateLegalMoveList(b, &moveList, 1);			

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		// Potential checkmate/stalemate found.  Score now.
		return determineEndOfGameState(b) == BOARD_CHECKMATE ? -9995 : 0;
	}
	
	for (byte ix = 0; ix < moveList.ix; ix++) {
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
			
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			// NOTE: scoringTeam == ~b->whosTurn
			score = analyseLeafNonTerminal(&(move->resultingBoard));
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Opponent's turn, and so will pick lowest score for us.
		//
		if (score < bestScore) {
			bestScore = score;
		}
	} // for ix

	// Return our rating of the move now living in bestMove.
	return bestScore;
}

//
// YOUR TURN: b->whosTurn
//
scoreType level2(	const board* const loopDetect, 
					const board* const b) {

	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		// Potential checkmate/stalemate found.  Score now.
		return determineEndOfGameState(b) == BOARD_CHECKMATE ? 9996 : 0;
	}
	
	for (byte ix = 0; ix < moveList.ix; ix++) {
		//
		// Assess the move [from]->[to] on board b to depth numMoves-1.
		//
		// We do this by looking at the resulting board and then seeing what score
		// we get when we follow all the moves from then on down to numMoves -1
		// and see if any of those leaf level boards has a higher best score
		// than what we have now.
		//
		analysisMove* move = &(moveList.items[ix]);
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			score = level3(loopDetect, &(move->resultingBoard));
		}
		
		//
		// Update bestscore to be the best score.
		//
		// If it's our turn, get the highest (MAX), if it's their turn, get the lowest (MIN)
		//
		if (score > bestScore) {
			bestScore = score;
		}
				
	} // for ix

	// Return our rating of the move now living in bestMove.
	return bestScore;
}

//
// OPPONENT TURN: ~b->whosTurn
//
scoreType level1(	const board* const loopDetect, 
					const board* const b) {

				
	// Start by assuming the best us, and see if the opponent can make it worse.
	scoreType bestScore = 9999;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			
	
	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		// Potential checkmate/stalemate found.  Score now.
		return determineEndOfGameState(b) == BOARD_CHECKMATE ? -9997 : 0;
	}
	
	for (byte ix = 0; ix < moveList.ix; ix++) {
		//
		// Assess the move [from]->[to] on board b to depth numMoves-1.
		//
		// We do this by looking at the resulting board and then seeing what score
		// we get when we follow all the moves from then on down to numMoves -1
		// and see if any of those leaf level boards has a higher best score
		// than what we have now.
		//
		analysisMove* move = &(moveList.items[ix]);
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			// If a loop is detected, don't recurse, just score very badly (for us)
			score = -9980;
		}
		else {
			score = level2(loopDetect, &(move->resultingBoard));
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Opponent's turn, and so will pick lowest score for us.
		//
		if (score < bestScore) {
			bestScore = score;
		}
				
	} // for ix

	// Return our rating of the move now living in bestMove.
	return bestScore;
}


//
// YOUR TURN: b->whosTurn
//
scoreType level0(	analysisMove* const bestMove, 
					const board* const loopDetect, 
					const board* const b) {
	
	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			

	// FAILSAFE ONLY - COULD REMOVE
	if (moveList.ix == 0) {
		error("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
	}
	
	for (byte ix = 0; ix < moveList.ix; ix++) {
		//
		// Assess the move [from]->[to] on board b to leaf level.
		//
		// We do this by looking at the resulting board and then seeing what score
		// we get when we follow all the moves from then on down to leaf level
		// and see if any of those leaf level boards has a higher best score
		// than what we have now.
		//
		analysisMove* move = &(moveList.items[ix]);
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			score = level1(loopDetect, &(move->resultingBoard));
		}
		
		//
		// Update bestscore to be the best score.
		//
		// If it's our turn, get the highest (MAX), if it's their turn, get the lowest (MIN)
		//
		if (score > bestScore) {
			
			bestScore = score;
			// Literally only useful at depth == 0;
			memcpy((void*)bestMove, (void*)move, sizeof(analysisMove));
		}
				
	} // for ix

	// Return our rating of the move now living in bestMove.
	return bestScore;
}


/*
scoreType getBestMove(analysisMove* const bestMove, const board* const loopDetect, const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth) {
				
	// Start by assuming the worst for us (or the best for the opponent), and see if we can do better than that.
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
			
			// If a loop is detected, don't recurse, just score very badly
			score = (b->whosTurn == scoringTeam) ? (-9998 + aiStrength + 1) : (9999 - aiStrength - 1);
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

	// Return our rating of the move now living in bestMove.
	return bestScore;
}
*/
