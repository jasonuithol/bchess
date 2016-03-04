
depthType determineAiStrength(const board* const current) {

	moveList myMoves, theirMoves;
	
	const scoreType material = evaluateMaterial(current,current->whosTurn, 1) 
			     + evaluateMaterial(current,opponentOf(current->whosTurn), 1);
		
	logg("Material score for determining ai strength: %d\n", material);	
	
	depthType strength = 8;
		
	// NOTE: Maximum material is 154, minimum is 0
	if (material > 154 || material < 0) {
		error("Bad absolute material value calculated: %d\n", material); 
	}
		
	if (material >= 50) {
		strength = 4;
	}
	else if (material >= 30) {
		strength = 5;
	}
	else if (material >= 15) {
		strength = 6;
	}
	else if (material >= 5) {
		strength = 7;
	}
	
	return strength;
}

void printReasoning(const analysisList* const bestAnalysis, const board* const current, const depthType aiStrength) {

	logg("Reasoning now being printed\n");
	for (depthType i = 0; i < aiStrength; i++) {
		scoreType mat, mob;				
		mat = evaluateMaterial(&(bestAnalysis->moves[i].mv.resultingBoard), current->whosTurn, 0);
		mob = evaluateMobility(&(bestAnalysis->moves[i].mv.resultingBoard), current->whosTurn);
//		ini = evaluateInitiative(&(bestAnalysis->moves[i].mv.resultingBoard), current->whosTurn);
//		logg("Depth %d, material %d, mobility %d, initiative %d\n", i + 1, mat, mob, ini);
		logg("Depth %d, material %d, mobility %d\n", i + 1, mat, mob);

//		printBoardToLog(&(bestAnalysis->moves[i].mv.resultingBoard));
		printQB(current->quad);

		if (detectCheckmate(&(bestAnalysis->moves[i].mv.resultingBoard))) {
			logg("CHECKMATE/STALEMATE\n");
			break;
		}
	}
	
}

//
// Ask an AI agent to make a move.
//
void aiMove(const board* const current, board* const next, const int turnNumber) {

	const time_t startTime = time(NULL);

	// I'm just sick of the queen based games.
	if (turnNumber > 5) {
		queenCanMove = 1;
	}
	else {
		queenCanMove = 0;
	}
	
	nodesCalculated = 0;
	analysisMove bestmove;
	const depthType aiStrength = determineAiStrength(current);
			
	print("Choosing aiStrength %d\n", aiStrength);
	
	// TODO: Pick an AI strategy and pass it into this method call.
	analysisList* bestAnalysis = getBestMove(&bestmove, current, current->whosTurn, aiStrength, aiStrength);

	// Print/log reasoning behind move.
	printReasoning(bestAnalysis, current, aiStrength);

	// deallocate the now useless history.
	free(bestAnalysis);

	print("\n");
	makeMove(current, next, bestmove.mv);

	const time_t finishTime = time(NULL);
	const double timetaken = difftime(finishTime, startTime);
	
	print("===== ai move for ");printTeam(current->whosTurn);
	print(" at ai strength %d =====\n", aiStrength);

	print("Move chosen: ");
	printMove(current, bestmove.mv);
	if (isKingChecked(next, next->whosTurn)) {
		print(" >>> CHECK <<<");
	}
	print(" (score: %d)\n", bestmove.score);
    print("Ai Move Time Taken: %f\n", timetaken);

//	printBoardUnicode(next);	
	printQB(next->quad);	
}
