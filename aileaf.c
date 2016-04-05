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

inline void displaySpinningPulse() {

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

inline scoreType evaluateMaterial(const board* const b) {

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



inline scoreType countLegalMoves(const board* const b, const byte team) {
	
	scoreType score = 0;
	
	attackContext ac;
	
	ac.hardBlockers = getTeamPieces(b->quad, team);
	ac.softBlockers = getTeamPieces(b->quad, team ^ 1);

	for (byte i = 0; i < 6; i++) {

		// Choose the appropriate vector and moves context for the piece type (i)
		// NOTE: i does NOT map to pieceType !
		const vectorsContext* vc = &(pieceVectorContexts[b->whosTurn][i]);
		const moveContext* mc = &(moveContexts[i]);
		
		// Get all the pieces for the pieceType (e.g. all the black bishops).
		iterator pieces = newIterator(getPieces(b->quad, vc->pieceType | b->whosTurn));
		pieces = getNextItem(pieces);
					
		while (pieces.item) {
			
			ac.piece = pieces.item;

			// Obtain a list of standard attacks for the piece.												
			score += populationCount(mc->moveGenerator(&ac, vc, b));
			
			// Get the next black bishop.
			pieces = getNextItem(pieces);
		}	
	}	
	
	return score;
}



//
// SLOW AS SHIT
//
inline scoreType evaluateMobility(const board* const b) {
	return countLegalMoves(b, b->whosTurn) 
		   - 
		   countLegalMoves(b, b->whosTurn ^ 1);
}

inline scoreType analyseLeafNonTerminal(const board* const b) {
	
	// Tell the world we still live.
	displaySpinningPulse();
		
	// We have hit the limit of our depth search - time to score the board.
	return (1 * evaluateMobility(b))
		 + (8 * evaluateMaterial(b));	

}



