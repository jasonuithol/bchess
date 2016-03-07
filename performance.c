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
	board b;	
	initBoard(&b);
	time_t startTime = time(NULL);
	if (getBestMove(&bestMove, &b, 0, 5, 0) == 99) {
		print("99 !!!!!\n");
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
	printf("bill oddie\n"); fflush(stdout);
	double a1 = perftest_displaySpinningPulse();
	printf("bill oddie\n"); fflush(stdout);
	double a2 = perftest_evaluateMaterial();
	printf("bill oddie\n"); fflush(stdout);
	double a3 = perftest_evaluateMobility_Empty();
	printf("bill oddie\n"); fflush(stdout);
	double a4 = perftest_evaluateMobility_Initial();
	printf("bill oddie\n"); fflush(stdout);
	double a5 = perftest_generateLegalMoveList_LeafMode();
	printf("bill oddie\n"); fflush(stdout);
	double a6 = perftest_isSquareAttacked();
	printf("bill oddie\n"); fflush(stdout);
	double a7 = perftest_getBestMove_Initial();
	printf("bill oddie\n"); fflush(stdout);
	
	print("Performance results: %f %f %f %f %f %f %f\n", a1, a2, a3, a4, a5, a6, a7);
}
