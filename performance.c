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
		if (evaluateMaterial(b.quad, WHITE) == 99) {
			print("99 !!!!!\n");
		}
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_evaluateMobility_Empty() {
	board b;
	clearBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < PERF_ITERATIONS; i++) {
		if (evaluateMobility(b.quad, WHITE) == 99) {
			print("99 !!!!!\n");
		}
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_evaluateMobility_Initial() {
	board b;
	initBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < PERF_ITERATIONS; i++) {
		if (evaluateMobility(b.quad, WHITE) == 99) {
			print("99 !!!!!\n");
		}
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
} 

double perftest_getBestMove_Initial() {
	analysisMove bestMove;
	board b, loopDetect;	
	initBoard(&b);
	clearBoard(&loopDetect);
	time_t startTime = time(NULL);
	if (getBestMove(&bestMove, &loopDetect, &b, 0, 4, 0) == 99) {
		print("99 !!!!!\n");
	}
	else {
		print("printing BestMove merged bitboard\n");
		printBB(bestMove.from | bestMove.to);
	}
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
		}
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
}

double perftest_isSquareAttacked() {
	board b;	
	initBoard(&b);
	time_t startTime = time(NULL);
	for (int i = 0; i < 64; i++) {
		if (isSquareAttacked(b.quad, 1ULL<< i, WHITE) == 99) {
			print("99 !!!!!\n");
		}
	}
	time_t finishTime = time(NULL);
	return difftime(finishTime, startTime);
}


void runPerformanceSuite() {

	double a1 = perftest_displaySpinningPulse();
	printf("perftest_displaySpinningPulse: %f\n", a1); fflush(stdout);

	double a2 = perftest_evaluateMaterial();
	printf("perftest_evaluateMaterial: %f\n", a2); fflush(stdout);

	double a3 = perftest_evaluateMobility_Empty();
	printf("perftest_evaluateMobility_Empty: %f\n", a3); fflush(stdout);

	double a4 = perftest_evaluateMobility_Initial();
	printf("perftest_evaluateMobility_Initial: %f\n", a4); fflush(stdout);

	double a5 = perftest_generateLegalMoveList_LeafMode();
	printf("perftest_generateLegalMoveList_LeafMode: %f\n", a5); fflush(stdout);

	double a6 = perftest_isSquareAttacked();
	printf("perftest_isSquareAttacked: %f\n", a6); fflush(stdout);

	double a7 = perftest_getBestMove_Initial();
	printf("perftest_getBestMove_Initial: %f\n", a7); fflush(stdout);
	
}
