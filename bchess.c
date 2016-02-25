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
#include "human.c"

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
//	crashTest(b1ptr);
	printAllowedMoves(b1ptr);
	printBoardClassic(b1ptr);
//	exit(1);

	printf("\n-------------- ai test ----------------\n");

	int turn;
	for (turn = 0; turn < 200; turn++) { // for this demo, limit to 50 moves.
		printf("==== TURN %d =====\n\n",turn);

		// For this demo, the AI plays black.
		if (b1ptr->whosTurn == WHITE) {
//			aiMove(b1ptr,b2ptr, turn);
			humanMove(b1ptr,b2ptr);
		}
		else {
			aiMove(b1ptr,b2ptr, turn);
		}

		// swap board pointers to make the new board the next old board.
		tempPtr = b1ptr;
		b1ptr = b2ptr;
		b2ptr = tempPtr;
		
		int gamestate = detectCheckmate(b1ptr);
		switch(gamestate) {
			case BOARD_CHECKMATE:
				printf("CHECKMATE !!! Victory to ");
				printTeam(teamOf(opponentOf(b1ptr->whosTurn)) );
				printf("\n");
				fflush(stdout);	
				exit(0);	
			case BOARD_STALEMATE:
				printf("STALEMATE !!! It's draw.\n");
				fflush(stdout);		
				exit(0);	
		}
		
	}

	time_t finishTime = time(NULL);
    printf("Overall Time Taken: %f\n", difftime(finishTime, startTime));
    return 0;
}
