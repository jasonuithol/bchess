//
// Ask an AI agent to make a move.
//
void aiMove(const board* const current, board* const next, const board* const loopDetectPtr, const int turnNumber) {

	const time_t startTime = time(NULL);
	
	// Seed the random number generator
	srand(startTime);
	
	nodesCalculated = 0;
//	scoreType score = getBestMove(&bestmove, loopDetectPtr, current, current->whosTurn, 4, 0);
	movePlan plan; 
	level0(loopDetectPtr, current, &plan, 1); // deep plan
	analysisMove* bestmove = &(plan.items[0]);

	print("\n");
	makeMove(current, next, bestmove);

	const time_t finishTime = time(NULL);
	const double timetaken = difftime(finishTime, startTime);
	
	print("===== ai move for %s\n", current->whosTurn ? "BLACK" : "WHITE");

	print("Move chosen: ");
	byte mover = trailingBit_Bitboard(bestmove->from);
	printPieceUnicode(getType(current->quad,mover), current->whosTurn, UNICODESET_SOLID);
	printResetColors();
	print(" ");
	bitboard taken = getAllPieces(current->quad) & bestmove->to;
	if (taken) {
		byte takenOffset = trailingBit_Bitboard(bestmove->to);
		print("x ");
		printPieceUnicode(getType(current->quad,takenOffset), opponentOf(current->whosTurn), UNICODESET_SOLID);
		printResetColors();
		print(" ");
	}
	
	printMove(*bestmove);

	if (isKingChecked(next->quad, next->whosTurn)) {
		print(" >>> CHECK <<<");
	}
	print(" (score: %d, nodes: %d)\n", (int)plan.score, (int)nodesCalculated);
    print("Ai Move Time Taken: %f, processing speed %f\n", timetaken, nodesCalculated / timetaken);

	printQBUnicode(next->quad);	
}
