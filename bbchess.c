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

/*
#include "airoot.c"
*/
#include "performance.c"

int main() {
	board b;
	initBoard(&b);
	
//	printf("setBitXY 0,0 and 7,7: %" PRIx64 " %" PRIx64 "\n", setBitXY(0,0), setBitXY(7,7));
	
//	int bbSize = sizeof(bitboard);
//	int ullSize = sizeof(unsigned long long);
//	int uiSize = sizeof(unsigned int);	
//	printf("Sizeof bitboard %d, sizeof unsigned long long %d, sizeof unsigned int %d\n", 
//	       bbSize, ullSize, uiSize);
	
//	printf("Number of pieces on the white teamboard %d\n", populationCount((bitboard*)&(b.white), sizeof(teamboard) / sizeof(bitboard) ));
	
//	printf("Value of white.bishops: %" PRIx64 "\n", b.white.bishops);
	
	printf("8ULL << 7 = %lld\n", 8ULL << 7);
	bitboard testb = 8;
	offset testo = 7;
	bitboard result = testb << testo;
	printf("testb << test0 = %" PRId64 "\n", result);
	printBB(result);
	

	print("WHITE King, Friends and Enemies\n");
	printBB(getPieces(b.quad , WHITE) | KING);   printf("\n");
	printBB(getTeamPieces(b.quad, WHITE)); printf("\n");
	printBB(getTeamPieces(b.quad, BLACK)); printf("\n");
	
	printf("Showing the whole board\n\n");
	printQB(b.quad);
	
	runPerformanceSuite();
	
//	printf("Print checking matrix for TEST\n\n");
//	bitboard testAttacks = generateTestCheckingMap(b.quad);
//	printBB(testAttacks);
	
	
//	printf("Showing the board with all the white pieces on it.\n\n");
//	printBB(unionTeamBoard(&(b.white)));
	
}
