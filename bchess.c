// ================================================================
//
// bchess.c
//
// The main() function is located here.
//

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "piece.c"
#include "board.c"
#include "moves.c"
#include "ai2.c"

int main() {

	// ----------------------
	//
	// EXECUTION STARTS HERE
	//
	// ----------------------

	time_t startTime = time(NULL);

	// Maintain pointers to a "current" board and a "next move" board.
	board b1,b2;
	board *b1ptr, *b2ptr, *tempPtr;
	b1ptr = &b1;
	b2ptr = &b2;

	initBoard(b1ptr);
	printAllowedMoves(b1ptr);

	printf("\n-------------- ai test ----------------\n");
	aiStrength = 3; // i.e. think that many moves ahead.

	int turn;
	for (turn = 0; turn < 200; turn++) { // for this demo, limit to 50 moves.
		printf("==== TURN %d =====\n\n",turn);

		// For this demo, the AI plays against itself.
		aiMove(b1ptr,b2ptr);

		// swap board pointers to make the new board the next old board.
		tempPtr = b1ptr;
		b1ptr = b2ptr;
		b2ptr = tempPtr;
	}

	time_t finishTime = time(NULL);
    printf("Overall Time Taken: %f\n", difftime(finishTime, startTime));
    return 0;
}
