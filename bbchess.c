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
//#include "loopdetect.c"




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

//	openLog();

	time_t startTime = time(NULL);

	board boards[5];

//	board loopHolder;

	byte current = 0;
	byte next = 1;
	byte loopDetectIx = 2;
	byte loopCount = 0;
	
	board* b1ptr = &(boards[current]);
	board* b2ptr = &(boards[next]);
	board* loopDetectPtr = &(boards[loopDetectIx]);



//#ifdef PROFILING_BUILD	
//	profilingBoard(b1ptr);
//#else
	initBoard(b1ptr);
//	clearBoard(&loopHolder);
//	crashTest(b1ptr);
//	pawnPromotionTest(b1ptr);
//#endif

//	printAllowedMoves(b1ptr);
//	printBoardUnicode(b1ptr);
	printQB(b1ptr->quad);
//	exit(1);

	print("\n-------------- ai test ----------------\n");

//#ifdef PROFILING_BUILD	
//	for (turn = 0; turn < 2; turn++) { 
//#else		
	for (byte turn = 0; turn <= 200; turn++) { 
//#endif		

		// For the first 3 moves, ensure no board will equal the loop detector, by clearing it.
		if (turn < 3) {
			clearBoard(loopDetectPtr);
		}

		print("==== TURN %d =====\n\n",turn);

//		print("\n\n\nLoop detect board: using loop index %u\n", loopDetectIx);
//		printQBUnicode(loopDetectPtr->quad);
		

//		printAllowedMoves(b1ptr);

		if (b1ptr->whosTurn == WHITE) {
//			aiMove(b1ptr,b2ptr,loopDetectPtr,turn);
			humanMove(b1ptr,b2ptr);
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
//				quit(0);	
				exit(0);
			case BOARD_STALEMATE:
				print("STALEMATE !!! It's a draw.\n");
//				quit(0);	
				exit(0);
		}

		if (areEqualQB(b2ptr->quad, loopDetectPtr->quad)) {

//			if (!areEqualQB(loopDetectPtr->quad, loopHolder.quad)) {
//				print("Resetting loop count due to new loop\n"); 
//				memcpy((void*)&loopHolder,(void*)b2ptr,sizeof(board));
//				loopCount = 0; 
//			}

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
    
//	quit(0);
}
#endif
