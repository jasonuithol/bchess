#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "logging.c"
#include "bitboard.c"
#include "iterator.c"
#include "quadboard.c"
#include "attacks.c"
#include "umpire.c"
#include "aileaf.c"
#include "ai2.c"
#include "airoot.c"
#include "human.c"

#ifdef BUILD_TESTING

#include "test.c"
#include "performance.c"


void diagnostics() {
	board b;
	initBoard(&b);
	
	
	printf("8ULL << 7 = %lld\n", 8ULL << 7);
	bitboard testb = 8;
	offset testo = 7;
	bitboard result = testb << testo;
	printf("testb << test0 = %" PRId64 "\n", result);
	printBB(result);
	

	print("WHITE King, Friends and Enemies\n");
	printBB(getPieces(b.quad , WHITE | KING));   printf("\n");
	printBB(getTeamPieces(b.quad, WHITE)); printf("\n");
	printBB(getTeamPieces(b.quad, BLACK)); printf("\n");
	
	print("Showing the whole board\n\n");
	printQB(b.quad);
	print("Showing board components. Type0\n");
	printBB(b.quad.type0);
	print("Type1\n");
	printBB(b.quad.type1);
	print("Type2\n");
	printBB(b.quad.type2);
	print("Team\n");
	printBB(b.quad.team);

	testSuite();	
	runPerformanceSuite();

}

int main() {
	diagnostics();
}

#else
int main() {

	// ----------------------
	//
	// EXECUTION STARTS HERE
	//
	// ----------------------

	time_t startTime = time(NULL);

	board boards[5];

	byte current = 0;
	byte next = 1;
	byte loopDetectIx = 2;
	byte loopCount = 0;
	
	board* b1ptr = &(boards[current]);
	board* b2ptr = &(boards[next]);
	board* loopDetectPtr = &(boards[loopDetectIx]);

	initBoard(b1ptr);
	printQB(b1ptr->quad);

	print("\n-------------- ai test ----------------\n");

	for (byte turn = 0; turn <= 200; turn++) { 

		// For the first 3 moves, ensure no board will equal the loop detector, by clearing it.
		if (turn < 3) {
			clearBoard(loopDetectPtr);
		}

		print("==== TURN %d =====\n\n",turn);

		if (b1ptr->whosTurn == WHITE) {
			aiMove(b1ptr,b2ptr,loopDetectPtr,turn);
//			humanMove(b1ptr,b2ptr);
		}
		else {
			aiMove(b1ptr,b2ptr,loopDetectPtr,turn);
		}
		
		int gamestate = detectCheckmate(b2ptr);
		
		switch(gamestate) {
			case BOARD_CHECKMATE:
				print("CHECKMATE !!! Victory to ");
				print(b1ptr->whosTurn ? "BLACK" : "WHITE");
				print("\n");
				exit(0);
			case BOARD_STALEMATE:
				print("STALEMATE !!! It's a draw.\n");
				exit(0);
		}

		if (areEqualQB(b2ptr->quad, loopDetectPtr->quad)) {

			loopCount++;
			
			if (loopCount >= 3) {
				print("DRAW BY 3 LOOPS !!!\n");
				exit(0);
			}
		}

		//
		// Increment the board indexes, and update the respective pointers.
		//
		current = (current + 1) % 5;
		next = (next + 1) % 5;
		loopDetectIx = (loopDetectIx + 1) % 5;

		b1ptr = &(boards[current]);
		b2ptr = &(boards[next]);
		loopDetectPtr = &(boards[loopDetectIx]);
				
	}

	time_t finishTime = time(NULL);
    printf("Overall Time Taken: %f\n", difftime(finishTime, startTime));
    
}
#endif
