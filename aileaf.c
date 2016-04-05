#define SCORE_PAWN 			(1)
#define SCORE_KNIGHT 		(4)
#define SCORE_BISHOP 		(6)
#define SCORE_ROOK 			(14)
#define SCORE_QUEEN 		(21)
//#define SCORE_KING_CHECK 	(10)

typedef uint32_t nodesCalculatedType;
typedef uint8_t depthType;
typedef int16_t scoreType;

//
// Animates a little spinner to show that we are still alive.
//
// IMPORTANT: Do not use print() !! Instead, use printf() 
// We don't want this going into the logs !!!!
//

nodesCalculatedType nodesCalculated;

#define PULSE_SPIN_MAGNITUDE (17)

void displaySpinningPulse() {

	const nodesCalculatedType wrapMask = (1 << (PULSE_SPIN_MAGNITUDE)) - 1;

	nodesCalculated++; 

	switch (nodesCalculated & wrapMask) {
		case 0:
			printf("/\b");
			fflush(stdout);
			break;
		case (1 << (PULSE_SPIN_MAGNITUDE - 2)):
			printf("-\b");
			fflush(stdout);
			break;
		case (1 << (PULSE_SPIN_MAGNITUDE - 1)):
			printf("\\\b");
			fflush(stdout);
			break;
		case (1 << (PULSE_SPIN_MAGNITUDE - 2)) | (1 << (PULSE_SPIN_MAGNITUDE - 1)):
			printf("|\b");
			fflush(stdout);
			break;
	}
}


//
// MATERIAL: A cheap and cheerful strategy for deciding the best move
//           based on how much material the player has at the end of 
//           each possible move.
//

scoreType evaluateMaterial(const board* const b) {

	return    SCORE_PAWN 	* populationCount(getPieces(b->quad, PAWN   | b->whosTurn))
			+ SCORE_KNIGHT	* populationCount(getPieces(b->quad, KNIGHT | b->whosTurn))
			+ SCORE_BISHOP	* populationCount(getPieces(b->quad, BISHOP | b->whosTurn))
			+ SCORE_ROOK	* populationCount(getPieces(b->quad, ROOK   | b->whosTurn))
			+ SCORE_QUEEN	* populationCount(getPieces(b->quad, QUEEN  | b->whosTurn))
			
			- SCORE_PAWN 	* populationCount(getPieces(b->quad, PAWN   | (b->whosTurn^1)))
			- SCORE_KNIGHT	* populationCount(getPieces(b->quad, KNIGHT | (b->whosTurn^1)))
			- SCORE_BISHOP	* populationCount(getPieces(b->quad, BISHOP | (b->whosTurn^1)))
			- SCORE_ROOK	* populationCount(getPieces(b->quad, ROOK   | (b->whosTurn^1)))
			- SCORE_QUEEN	* populationCount(getPieces(b->quad, QUEEN  | (b->whosTurn^1))) 
	;			
}


//
// SLOW AS SHIT
//
scoreType evaluateMobility(const board* const b) {

	return 0;

	analysisList mine; mine.ix = 0;
	analysisList theirs; theirs.ix = 0;
		
	board theirTurnBoard;
	memcpy((void*)&theirTurnBoard, (void*)b, sizeof(board));
	theirTurnBoard.whosTurn = b->whosTurn ^ 1;	
		
	generateLegalMoveList(b, &mine, 1);
	generateLegalMoveList(&theirTurnBoard, &theirs, 1);
	
	return mine.ix - theirs.ix;
}

scoreType analyseLeafNonTerminal(const board* const b) {
	
	// Tell the world we still live.
	displaySpinningPulse();
		
	// We have hit the limit of our depth search - time to score the board.
	return (1 * evaluateMobility(b))
		 + (8 * evaluateMaterial(b));	

}



