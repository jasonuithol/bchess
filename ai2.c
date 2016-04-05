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

#define SECOND_ANALYSIS_SIZE 	(10)
#define BEST_PLAN_CUTOFF 		(3)
#define WORST_PLAN_CUTOFF 		(3)

inline void addMoveToPlan(movePlan* const plan, const analysisMove* const move, byte planIx) {
	memcpy((void*)&(plan->items[planIx]), (void*)move, sizeof(analysisMove));
}

inline void addMoveToPlan_NULL(movePlan* const plan, byte planIx) {
	memset((void*)&(plan->items[planIx]), 0, sizeof(analysisMove));
}


void printMovePlan(const movePlan* const plan) {
	for (byte j = 0; j < 4; j++) {
		if (plan->items[j].from == 0 && plan->items[j].to == 0) {
			print("FINISH! ");
			break;
		}
		else {
			printMove(plan->items[j]);
			print(" ");
		}
	}
}

//
// OPPONENT TURN: ~b->whosTurn
//
inline void level3(	const board* const loopDetect, 
				const board* const b,
				movePlan* const plan) {

	const byte planIx = 3;

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
		return;
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

}

//
// YOUR TURN: b->whosTurn
//
void level2(	const board* const loopDetect, 
				const board* const b,
				movePlan* const plan) {

	const byte planIx = 2;

	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;

	int bestIx = -1;
	analysisList moveList;
	moveList.ix = 0; // MANDATORY

	movePlan candidatePlans[ANALYSIS_SIZE];
	
	
	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			

	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {

		// Potential checkmate/stalemate found.  Score now.
		plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? -9996 : 0;

		// Add the NULL move to this plan slot.
		addMoveToPlan_NULL(plan, planIx);
		
		// Stop thinking, and report back with our plan.
		return;		
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
		movePlan* candidate = &(candidatePlans[ix]);
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			level3(loopDetect, &(move->resultingBoard), candidate);
			score = candidate->score;
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Our turn, so pick the highest.
		//
		if (score > bestScore) {
			bestScore = score;
			bestIx = ix;
		}
		
	} // for ix

	// Update the moveplan.
	plan->score = bestScore;
	addMoveToPlan(plan, &(moveList.items[bestIx]), planIx);
	addMoveToPlan(plan, &(candidatePlans[bestIx].items[planIx + 1]), planIx + 1);

}

//
// OPPONENT TURN: ~b->whosTurn
//
void level1(	const board* const loopDetect, 
				const board* const b,
				movePlan* const plan	) {

	const byte planIx = 1;

	// Start by assuming the best us, and see if the opponent can make it worse.
	scoreType bestScore = 9999;

	int bestIx = -1;
	analysisList moveList;
	moveList.ix = 0; // MANDATORY
	
	movePlan candidatePlans[ANALYSIS_SIZE];

	// Do non-leaf analysis
	generateLegalMoveList(b, &moveList, 0);			
	
	// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
	if (moveList.ix == 0) {

		// Potential checkmate/stalemate found.  Score now.
		plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? 9997 : 0;

		// Add the NULL move to this plan slot.
		addMoveToPlan_NULL(plan, planIx);
		
		// Stop thinking, and report back with our plan.
		return;
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
		movePlan* candidate = &(candidatePlans[ix]);
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			level2(loopDetect, &(move->resultingBoard), candidate);
			score = candidate->score;
		}
		
		//
		// Update bestscore to be the best score.
		//
		// Opponent's turn, and so will pick lowest score for us.
		//
		if (score < bestScore) {
			bestScore = score;
			bestIx = ix;			
		}
				
	} // for ix

	// Update the moveplan.
	plan->score = bestScore;
	addMoveToPlan(plan, &(moveList.items[bestIx]), planIx);
	addMoveToPlan(plan, &(candidatePlans[bestIx].items[planIx + 1]), planIx + 1);
	addMoveToPlan(plan, &(candidatePlans[bestIx].items[planIx + 2]), planIx + 2);

}



int level0toplevel(	const board* const loopDetect, 
					const board* const b,
					movePlan plans[ANALYSIS_SIZE],
					byte numMoves);

//
// YOUR TURN: b->whosTurn
//
void level0(	const board* const loopDetect, 
				const board* const b,
				movePlan* const plan,
				const byte toplevel) {

//	print("level0 handed the following board\n");
//	printQB(b->quad);

	const byte planIx = 0;

	// Start by assuming the worst for us and see if we can do better than that.
	scoreType bestScore = -9999;

	int bestIx = -1;
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
			plan->score = determineEndOfGameState(b) == BOARD_CHECKMATE ? -9998 : 0;

			// Add the NULL move to this plan slot.
			addMoveToPlan_NULL(plan, planIx);
			
			// Stop thinking, and report back with our plan.
			return;				
		}
	}
	
	// In this level, we keep a list of pointers to ALL plans.
	// However, this is only useful for toplevel
	movePlan candidatePlans[ANALYSIS_SIZE];
		
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
		movePlan* candidate = &(candidatePlans[ix]);
		scoreType score;
		
		if (areEqualQB(loopDetect->quad, move->resultingBoard.quad)) {
			// If a loop is detected, don't recurse, just score very badly
			score = -9980;
		}
		else {
			level1(loopDetect, &(move->resultingBoard), candidate);
			score = candidate->score;
		}

		if (toplevel) {
			// At toplevel, we track multiple plans.
			candidate->score = score;
			addMoveToPlan(candidate, move, planIx);
		}
		else {
			// At deep level, we look for the best plan.
			if (score > bestScore) {
				bestScore = score;
				bestIx = ix;
			}
		}
						
	} // for ix


	if (toplevel) {
		
		// We need to go deeper.
		bestIx = level0toplevel(	loopDetect, 
									b,
									candidatePlans,
									moveList.ix	);
									
		// Grab the deeper score.							
		bestScore = candidatePlans[bestIx].score;

	}

	// Update the moveplan.
	plan->score = bestScore;
	addMoveToPlan(plan, &(moveList.items[bestIx]), planIx);
	addMoveToPlan(plan, &(candidatePlans[bestIx].items[planIx + 1]), planIx + 1);
	addMoveToPlan(plan, &(candidatePlans[bestIx].items[planIx + 2]), planIx + 2);
	addMoveToPlan(plan, &(candidatePlans[bestIx].items[planIx + 3]), planIx + 3);

}


//
// YOUR TURN: b->whosTurn
//
int level0toplevel(		const board* const loopDetect, 
						const board* const b,
						movePlan plans[ANALYSIS_SIZE],
						byte numMoves			) {

	scoreType bestScores[SECOND_ANALYSIS_SIZE];
	int bestMoveIx[SECOND_ANALYSIS_SIZE];

	// Assume the worst
	for (byte j = 0; j < SECOND_ANALYSIS_SIZE; j++) {
		bestScores[j] = -9999;
		bestMoveIx[j] = -1;
	}
	// For the worst moves, actually assume the best instead.
	for (byte j = BEST_PLAN_CUTOFF; j < BEST_PLAN_CUTOFF + WORST_PLAN_CUTOFF; j++) {
		bestScores[j] = 9999;
	}


	//
	// Find BEST_PLAN_CUTOFF of the best moves.
	//
	for (int ix = 0; ix < numMoves; ix++) {

		movePlan* plan = &(plans[ix]);
		
		// Look for a best slot that we are better than
		for (byte j = 0; j < BEST_PLAN_CUTOFF && j < numMoves; j++) {
			
			if (plan->score > bestScores[j]) {  // FINDING THE BEST
				
				bestScores[j] = plan->score;
				bestMoveIx[j] = ix;
				
				// Only overwrite one slot, not all.
				break;
			}
		}
	}
	
	
	//
	// Find WORST_PLAN_CUTOFF of the worst moves.
	//
	for (int ix = 0; ix < numMoves; ix++) {

		movePlan* plan = &(plans[ix]);
		
		// Look for a slot that we are worse than
		for (byte j = BEST_PLAN_CUTOFF; j < BEST_PLAN_CUTOFF + WORST_PLAN_CUTOFF && j < numMoves; j++) {
			
			if (plan->score < bestScores[j] && plan->score > -9000) {  // FINDING THE WORST
				
				bestScores[j] = plan->score;
				bestMoveIx[j] = ix;
				
				// Only overwrite one slot, not all.
				break;
			}
		}
	}
	
	//
	// Fill the remainder with random picks that aren't already chosen.
	//
	int remainingIx = BEST_PLAN_CUTOFF + WORST_PLAN_CUTOFF + 1;
		
	while (remainingIx < SECOND_ANALYSIS_SIZE && remainingIx < numMoves) {
		
		int randIx = rand() % numMoves;
		int isUnique = 1;
		
		for (int j = 0; j < remainingIx; j++) {
			if (randIx == bestMoveIx[j]) {
				isUnique = 0;
				break;
			}
		}

		if (isUnique && plans[randIx].score > -9000) {
			bestMoveIx[remainingIx] = randIx;
			remainingIx++;
		}
	}

	//
	// Further analyse remaining moves
	//
	
	scoreType bestScore = -9999;
	movePlan candidateDeepPlans[SECOND_ANALYSIS_SIZE];
	int bestIx = -1;

	for (byte j = 0; j < SECOND_ANALYSIS_SIZE; j++) {

		int ix = bestMoveIx[j];

		// It's possible to have less than SECOND_ANALYSIS_SIZE moves to consider.
		if (ix == -1) {
			continue;
		}
		
		movePlan* candidatePlan = &(plans[ix]);
		analysisMove* deepMove = &(candidatePlan->items[3]);

		scoreType score;
		
		print("Plan %d scored %d: ", (int)j, candidatePlan->score);
		printMovePlan(candidatePlan);
		
		if (candidatePlan->score < 9000 && candidatePlan->score > -9000) {

			movePlan* candidateDeepPlan = &(candidateDeepPlans[j]);

			//
			// Drill down 4 more levels on this candidate move.
			//
			level0(		loopDetect, 
						&(deepMove->resultingBoard),
						candidateDeepPlan, 
						0					);
							
			score = candidateDeepPlan->score;
			
			// Might as well pass the deep score back up to the caller.
			candidatePlan->score = score;
			
			printMovePlan(candidateDeepPlan);
			print("now scoring %d\n", score);
			
			
		}
		else {
			// Was a terminal move (checkmate or stalemate) so 
			// do not try to drill down any further.
			score = candidatePlan->score;
			print(" not analysing terminal node further.\n");
		}
		
//		print("now scoring %d\n", score);
					
		if (score > bestScore) {
			bestScore = score;
			bestIx = ix;
		}
		
	}

	// Return the index to the best plan we found.
	return bestIx;
}

