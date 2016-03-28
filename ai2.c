// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//

//
// "Recursively" search for the best move and set "bestMove" to point to it.
//



typedef struct {
	analysisMove items[4];
	scoreType score;
} movePlan;

#define SECOND_ANALYSIS_SIZE (3)


//
// OPPONENT TURN: ~b->whosTurn
//
scoreType level3(	const board* const loopDetect, 
					const board* const b,
					movePlan* const plan) {

	// Start by assuming the best us, and see if the opponent can make it worse.
	scoreType bestScore = 9999;
	analysisMove* bestMove = NULL;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do LEAF analysis !!! MUCH LIGHTER LOAD (No castling rights checked)
	generateLegalMoveList(b, &moveList, 0);			

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		// Potential checkmate/stalemate found.  Score now.
		return determineEndOfGameState(b) == BOARD_CHECKMATE ? 9995 : 0;
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
//		analysisMove dummyMove;
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
			bestMove = move;
			
		}
	} // for ix


	// Update the moveplan.
	memcpy((void*)&(plan->items[3]), (void*)bestMove, sizeof(analysisMove));			
	plan->score = bestScore;


	// Return our rating of the move now living in bestMove.
	return bestScore;
}

//
// YOUR TURN: b->whosTurn
//
scoreType level2(	const board* const loopDetect, 
					const board* const b,
					movePlan* const plan) {

	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;
	analysisMove* bestMove = NULL;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		// Potential checkmate/stalemate found.  Score now.
		return determineEndOfGameState(b) == BOARD_CHECKMATE ? -9996 : 0;
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
			score = level3(loopDetect, &(move->resultingBoard), plan);
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Our turn, so pick the highest.
		//
		if (score > bestScore) {

			bestScore = score;
			bestMove = move;
		}
	} // for ix

	// Also update the moveplan.
	memcpy((void*)&(plan->items[2]), (void*)bestMove, sizeof(analysisMove));
	plan->score = bestScore;

	// Return our rating of the move now living in bestMove.
	return bestScore;
}

//
// OPPONENT TURN: ~b->whosTurn
//
scoreType level1(	const board* const loopDetect, 
					const board* const b,
					movePlan* const plan) {

				
	// Start by assuming the best us, and see if the opponent can make it worse.
	scoreType bestScore = 9999;
	analysisMove* bestMove = NULL;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			
	
	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {
		
		// Potential checkmate/stalemate found.  Score now.
		return determineEndOfGameState(b) == BOARD_CHECKMATE ? 9997 : 0;
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
			score = level2(loopDetect, &(move->resultingBoard), plan);
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Opponent's turn, and so will pick lowest score for us.
		//
		if (score < bestScore) {
			
			bestScore = score;
			bestMove = move;			
		}
				
	} // for ix

	// Also update the moveplan.
	memcpy((void*)&(plan->items[1]), (void*)bestMove, sizeof(analysisMove));
	plan->score = bestScore;

	// Return our rating of the move now living in bestMove.
	return bestScore;
}



scoreType level0toplevel(	analysisMove* const bestMove, 
							const board* const loopDetect, 
							const board* const b,
							movePlan plans[ANALYSIS_SIZE],
							analysisList moveList);

//
// YOUR TURN: b->whosTurn
//
scoreType level0(	analysisMove* const bestMove, 
					const board* const loopDetect, 
					const board* const b,
					const byte toplevel) {
	
	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;
	movePlan* bestPlan = NULL;
	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			

	// FAILSAFE ONLY - COULD REMOVE
	if (moveList.ix == 0) {
		if (toplevel) {
			error("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
		}
		else {
			// Potential checkmate/stalemate found.  Score now.
			return determineEndOfGameState(b) == BOARD_CHECKMATE ? -9998 : 0;
		}
	}
	
	
	movePlan plans[ANALYSIS_SIZE];
		
	//
	// Score each move at a depth of 4 plys (2 full turns)
	//
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
		movePlan* plan = &(plans[ix]);
		scoreType score;
		
//		if (areEqualQB(move->resultingBoard.quad, move->resultingBoard.quad)) {
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			score = level1(loopDetect, &(move->resultingBoard), plan);
		}

		// ALWAYS Update the moveplan.
		memcpy((void*)&(plan->items[0]), (void*)move, sizeof(analysisMove));
		plan->score = score;

		if (score > bestScore) {
			// Only useful in deep analysis.
			bestScore = score;
			bestPlan = plan;
		}
						
	} // for ix

/*
	if (toplevel) {
		
		bestScore = level0toplevel(	bestMove, 
									loopDetect, 
									b,
									plans,
									moveList	);

	}
	else {
		error("FOR NOW, NOT DOING DEEP ANALYSIS");
		for (byte j = 0; j < 4; j++) {
			printMove(bestPlan->items[j]);
			print(" ");
		}		
	}
*/

	for (byte j = 0; j < 4; j++) {
		printMove(bestPlan->items[j]);
		print(" ");
	}
	print("\n");
	for (byte j = 0; j < 4; j++) {
		printQB(bestPlan->items[j].resultingBoard.quad);
		print("\n");
	}

	// Return our rating of the move now living in bestMove.
	// (only useful in deep analysis)
	return bestScore;
}


//
// YOUR TURN: b->whosTurn
//
scoreType level0toplevel(	analysisMove* const bestMove, 
							const board* const loopDetect, 
							const board* const b,
							movePlan plans[ANALYSIS_SIZE],
							analysisList moveList			) {

	scoreType bestScores[SECOND_ANALYSIS_SIZE];
	int bestMoveIx[SECOND_ANALYSIS_SIZE];

	for (byte j = 0; j < SECOND_ANALYSIS_SIZE; j++) {
		bestScores[j] = -9999;
		bestMoveIx[j] = -1;
	}

	//
	// Find the best moves
	//
	for (int ix = 0; ix < moveList.ix; ix++) {

		movePlan* plan = &(plans[ix]);
		
		// Look for a best slot that we are better than
		for (byte j = 0; j < SECOND_ANALYSIS_SIZE; j++) {
			
			if (plan->score > bestScores[j]) {

//					print("Found new potential candidate move at score %d: ", plan->score);
//					printMove(plan->items[0]);
//					print("\n");
				
				bestScores[j] = plan->score;
				bestMoveIx[j] = ix;
				
				// Only overwrite one slot, not all.
				break;
			}
		}
		
	}

	//
	// Further analyse remaining moves
	//
	
	scoreType bestScore = -9999;
//		movePlan deepPlans[SECOND_ANALYSIS_SIZE];

	for (byte j = 0; j < SECOND_ANALYSIS_SIZE; j++) {

		int bestIx = bestMoveIx[j];

		// It's possible to have less than SECOND_ANALYSIS_SIZE moves to consider.
		if (bestIx == -1) {
			continue;
		}
		
		movePlan* bestPlan = &(plans[bestIx]);
		analysisMove* deepMove = &(bestPlan->items[3]);

		scoreType score;
		
		print("Candidate plan at current score %d\n", bestPlan->score);
		for (byte j = 0; j < 4; j++) {
			printMove(bestPlan->items[j]);
			print(" ");
		}

//		if (bestPlan->score < 9000 || bestPlan->score > -9000) {

			//
			// Drill down 4 more levels on this candidate move.
			//
//			score = level0(	bestMove, 	// <-- We throw away the result here
//							loopDetect, 
//							&(deepMove->resultingBoard), 
//							0);


//		}
//		else {
			// Was a terminal move (checkmate or stalemate) so 
			// do not try to drill down any further.
			score = bestPlan->score;
//		}
		
		print("now scoring %d\n", score);
					
		if (score > bestScore) {
			
			bestScore = score;
			
			// We have a potential winner - update the bestMove
			memcpy((void*)bestMove, (void*)&(bestPlan->items[0]), sizeof(analysisMove));
		}
		
	}
	
	return bestScore;
}

