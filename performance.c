#define PERF_ITERATIONS (1000000)

double perftest_displaySpinningPulse() {
	time_t startTime = time(NULL);
	for (int i = 0; i < PERF_ITERATIONS; i++) {
		displaySpinningPulse();
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_evaluateMaterial() {
	board b;
	initBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < PERF_ITERATIONS; i++) {
		evaluateMaterial(b.quad, WHITE);
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_evaluateMobility_Empty() {
	board b;
	clearBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < PERF_ITERATIONS; i++) {
		evaluateMobility(b.quad, WHITE);
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_evaluateMobility_Initial() {
	board b;
	initBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < PERF_ITERATIONS; i++) {
		evaluateMobility(b.quad, WHITE);
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_getBestMove_Initial() {
	analysisMove bestMove;
	board b;	
	initBoard(&b);
	time_t startTime = time(NULL);
	getBestMove(&bestMove, &b, 0, 5, 0);
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
}

double perftest_generateLegalMoveList_LeafMode() {
	board b;	
	initBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < (PERF_ITERATIONS); i++) {
		analysisList moveList;
		moveList.ix = 0;
		generateLegalMoveList(&b, &moveList, 1);
		for (int ix = 0; ix < moveList.ix; ix++) {
			if (moveList.items[ix].from == 99) {
				print("99 !!!!!\n");
			}
//			evaluateMaterial(moveList.items[ix].resultingBoard.quad, moveList.items[ix].resultingBoard.whosTurn);
		}
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
}


void runPerformanceSuite() {
	double a1 = perftest_displaySpinningPulse();
	double a2 = perftest_evaluateMaterial();
	double a3 = perftest_evaluateMobility_Empty();
	double a4 = perftest_evaluateMobility_Initial();
	double a5 = perftest_generateLegalMoveList_LeafMode();
//	double a6 = perftest_getBestMove_Initial();
	
	print("Performance results: %f %f %f %f %f\n", a1, a2, a3, a4, a5);
}
