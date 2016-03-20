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
//#include "board.c"
#include "attacks.c"
#include "umpire.c"
#include "aileaf.c"
#include "ai2.c"
#include "airoot.c"


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

	// Maintain pointers to a "current" board and a "next move" board.
	board b1,b2;
	board *b1ptr, *b2ptr, *tempPtr;
	b1ptr = &b1;
	b2ptr = &b2;

//#ifdef PROFILING_BUILD	
//	profilingBoard(b1ptr);
//#else
	initBoard(b1ptr);
//	crashTest(b1ptr);
//	pawnPromotionTest(b1ptr);
//#endif

//	printAllowedMoves(b1ptr);
//	printBoardUnicode(b1ptr);
	printQB(b1ptr->quad);
//	exit(1);

	print("\n-------------- ai test ----------------\n");

	int turn;
//#ifdef PROFILING_BUILD	
//	for (turn = 0; turn < 2; turn++) { 
//#else		
	for (turn = 0; turn <= 200; turn++) { 
//#endif		
		print("==== TURN %d =====\n\n",turn);

//		printAllowedMoves(b1ptr);

		if (b1ptr->whosTurn == WHITE) {
			aiMove(b1ptr,b2ptr, turn);
//			humanMove(b1ptr,b2ptr);
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
				print("CHECKMATE !!! Victory to ");
				print(b2ptr->whosTurn ? "BLACK" : "WHITE");
				print("\n");
//				quit(0);	
				exit(0);
			case BOARD_STALEMATE:
				print("STALEMATE !!! It's a draw.\n");
//				quit(0);	
				exit(0);
		}
		
	}

	time_t finishTime = time(NULL);
    printf("Overall Time Taken: %f\n", difftime(finishTime, startTime));
    
//	quit(0);
}
#endif
