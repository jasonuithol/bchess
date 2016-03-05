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



void runPerformanceSuite() {
	double a1 = perftest_displaySpinningPulse();
	double a2 = perftest_evaluateMaterial();
	double a3 = perftest_evaluateMobility_Empty();
	double a4 = perftest_evaluateMobility_Initial();
	
	printf("Performance results: %f %f %f %f\n", a1, a2, a3, a4);
}
