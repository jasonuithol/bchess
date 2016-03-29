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

#define SECOND_ANALYSIS_SIZE (4)

void addMoveToPlan(movePlan* const plan, const analysisMove* const move, byte planIx) {
	memcpy((void*)&(plan->items[planIx]), (void*)move, sizeof(analysisMove));
}

void addMoveToPlan_NULL(movePlan* const plan, byte planIx) {
	memset((void*)&(plan->items[planIx]), 0, sizeof(analysisMove));
}

//
// OPPONENT TURN: ~b->whosTurn
//
movePlan* level3(	const board* const loopDetect, 
					const board* const b) {

	const byte planIx = 3;

	// MALLOC: This could be the chosen plan !
	movePlan* plan = malloc(sizeof(movePlan));

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
		plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? 9995 : 0;

		// Add the NULL move to this plan slot.
		addMoveToPlan_NULL(plan, planIx);
		
		// Stop thinking, and report back with our plan.
		return plan;
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
	plan->score = bestScore;
	addMoveToPlan(plan, bestMove, planIx);

	// Return our plan.
	return plan;
}

//
// YOUR TURN: b->whosTurn
//
movePlan* level2(	const board* const loopDetect, 
					const board* const b) {

	const byte planIx = 2;

	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;

	movePlan* bestPlan = NULL;
	analysisMove* bestMove = NULL;	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY

	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {

		// MALLOC: This could be the chosen plan !
		movePlan* plan = malloc(sizeof(movePlan));

		// Potential checkmate/stalemate found.  Score now.
		plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? -9996 : 0;

		// Add the NULL move to this plan slot.
		addMoveToPlan_NULL(plan, planIx);
		
		// Stop thinking, and report back with our plan.
		return plan;		
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
		movePlan* candidate = NULL;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			// If a loop is detected, don't recurse, just score very badly
			candidate = malloc(sizeof(movePlan));
			score = -9980;
		}
		else {
			candidate = level3(loopDetect, &(move->resultingBoard));
			score = candidate->score;
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Our turn, so pick the highest.
		//
		if (score > bestScore) {

			if (bestPlan != NULL) {
				// We're not going to use this plan, so discard.
				free(bestPlan);
			}

			bestScore = score;
			bestMove = move;
			bestPlan = candidate;
		}
		else {
			// We're not going to use this plan, so discard.
			free(candidate);
		}
	} // for ix

	// Update the moveplan.
	bestPlan->score = bestScore;
	addMoveToPlan(bestPlan, bestMove, planIx);

	// Return our plan.
	return bestPlan;
}

//
// OPPONENT TURN: ~b->whosTurn
//
movePlan* level1(	const board* const loopDetect, 
					const board* const b) {

	const byte planIx = 1;

	// Start by assuming the best us, and see if the opponent can make it worse.
	scoreType bestScore = 9999;

	movePlan* bestPlan = NULL;
	analysisMove* bestMove = NULL;	
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			
	
	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {

		// MALLOC: This could be the chosen plan !
		movePlan* plan = malloc(sizeof(movePlan));

		// Potential checkmate/stalemate found.  Score now.
		plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? 9997 : 0;

		// Add the NULL move to this plan slot.
		addMoveToPlan_NULL(plan, planIx);
		
		// Stop thinking, and report back with our plan.
		return plan;				
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
		movePlan* candidate = NULL;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			// If a loop is detected, don't recurse, just score very badly
			candidate = malloc(sizeof(movePlan));
			score = -9980;
		}
		else {
			candidate = level2(loopDetect, &(move->resultingBoard));
			score = candidate->score;
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Opponent's turn, and so will pick lowest score for us.
		//
		if (score < bestScore) {

			if (bestPlan != NULL) {
				// We're not going to use this plan, so discard.
				free(bestPlan);
			}
			
			bestScore = score;
			bestMove = move;			
			bestPlan = candidate;
		}
		else {
			// We're not going to use this plan, so discard.
			free(candidate);
		}
				
	} // for ix

	// Update the moveplan.
	bestPlan->score = bestScore;
	addMoveToPlan(bestPlan, bestMove, planIx);

	// Return our plan.
	return bestPlan;
}



movePlan* level0toplevel(	const board* const loopDetect, 
							const board* const b,
							movePlan* plans[ANALYSIS_SIZE],
							analysisList moveList);

//
// YOUR TURN: b->whosTurn
//
movePlan* level0(	const board* const loopDetect, 
					const board* const b,
					const byte toplevel) {

//	print("level0 handed the following board\n");
//	printQB(b->quad);

	const byte planIx = 0;

	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;

	movePlan* bestPlan = NULL;
	analysisMove* bestMove = NULL;	
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
			// MALLOC: This could be the chosen plan !
			movePlan* plan = malloc(sizeof(movePlan));

			// Potential checkmate/stalemate found.  Score now.
			plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? -9998 : 0;

			// Add the NULL move to this plan slot.
			addMoveToPlan_NULL(plan, planIx);
			
			// Stop thinking, and report back with our plan.
			return plan;				
		}
	}
	
	// In this level, we keep a list of pointers to ALL plans.
	// However, this is only useful for toplevel
	movePlan* candidates[ANALYSIS_SIZE];
		
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
		scoreType score;
		movePlan* candidate = NULL;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			// If a loop is detected, don't recurse, just score very badly
			candidate = malloc(sizeof(movePlan));
			score = -9980;
		}
		else {
			candidate = level1(loopDetect, &(move->resultingBoard));
//			candidate = level3(loopDetect, &(move->resultingBoard));
			score = candidate->score;
		}

		if (toplevel) {
			// At toplevel, we track multiple plans.
			candidate->score = score;
			addMoveToPlan(candidate, move, planIx);
			candidates[ix] = candidate;
		}
		else {
			// At deep level, we look for the best plan.
			// (and chuck away the rest)
			if (score > bestScore) {
				
				if (bestPlan != NULL) {
					// We're not going to use this plan, so discard.
					free(bestPlan);
				}

				bestScore = score;
				bestMove = move;
				bestPlan = candidate;
			}
			else {
				free(candidate);
			}
		}
						
	} // for ix


	if (toplevel) {
		
		bestPlan = level0toplevel(	loopDetect, 
									b,
									candidates,
									moveList	);

		for (byte ix = 0; ix < moveList.ix; ix++) {
			if (candidates[ix] != bestPlan) {
				// Don't need any candidate plans that aren't the best plan
//				print("About to free candidates[ix] in level0toplevel\n");
				free(candidates[ix]);
			}
		}
	}
	else {

		// Update the moveplan.
		bestPlan->score = bestScore;
		addMoveToPlan(bestPlan, bestMove, planIx);	
	}

//	for (byte j = 0; j < 4; j++) {
//		printMove(bestPlan->items[j]);
//		print(" ");
//	}
//	print("\n");
//	for (byte j = 0; j < 4; j++) {
//		printQB(bestPlan->items[j].resultingBoard.quad);
//		print("\n");
//	}

	// Return our rating of the move now living in bestMove.
	// (only useful in deep analysis)
	return bestPlan;
}


//
// YOUR TURN: b->whosTurn
//
movePlan* level0toplevel(	const board* const loopDetect, 
							const board* const b,
							movePlan* plans[ANALYSIS_SIZE],
							analysisList moveList			) {

	movePlan* veryBestPlan = NULL;
	scoreType bestScores[SECOND_ANALYSIS_SIZE];
	int bestMoveIx[SECOND_ANALYSIS_SIZE];

	// Assume the worst
	for (byte j = 0; j < SECOND_ANALYSIS_SIZE; j++) {
		bestScores[j] = -9999;
		bestMoveIx[j] = -1;
	}

	//
	// Find the best moves
	//
	for (int ix = 0; ix < moveList.ix; ix++) {

		movePlan* plan = plans[ix];
		
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
		
		movePlan* bestPlan = plans[bestIx];
		analysisMove* deepMove = &(bestPlan->items[3]);

		scoreType score;
		
//		print("Candidate plan at current score %d\n", bestPlan->score);
//		for (byte j = 0; j < 4; j++) {
//			printMove(bestPlan->items[j]);
//			print(" ");
//		}

		
		if (bestPlan->score < 9000 || bestPlan->score > -9000) {

			movePlan* candidateDeepPlan;

			//
			// Drill down 4 more levels on this candidate move.
			//
			candidateDeepPlan = level0(	loopDetect, 
							&(deepMove->resultingBoard), 
							0);
							
			score = candidateDeepPlan->score;

			// we only really need the score.
//			print("About to free candidateDeepPlan in level0toplevel\n");
			free(candidateDeepPlan);

		}
		else {
			// Was a terminal move (checkmate or stalemate) so 
			// do not try to drill down any further.
			score = bestPlan->score;
		}
		
//		print("now scoring %d\n", score);
					
		if (score > bestScore) {
			
//			if (veryBestPlan != NULL) {
				// Don't need this plan anymore.
//				print("About to free old veryBestPlan in level0toplevel\n");
//				free(veryBestPlan);
//			}
						
			bestScore = score;
			veryBestPlan = bestPlan;
		}
		
	}
	
	veryBestPlan->score = bestScore; // For display purposes only.
	return veryBestPlan;
}

