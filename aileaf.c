
#define SCORE_PAWN 			(1)
#define SCORE_KNIGHT 		(4)
#define SCORE_BISHOP 		(6)
#define SCORE_ROOK 			(14)
#define SCORE_QUEEN 		(21)
//#define SCORE_KING_CHECK 	(10)

typedef uint32_t nodesCalculatedType;
typedef uint8_t depthType;

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

scoreType evaluateMaterial(const quadboard qb, const byte whosTurn, const byte opponent) {

	return    SCORE_PAWN 	* populationCount(getPieces(qb, PAWN   | whosTurn))
			+ SCORE_KNIGHT	* populationCount(getPieces(qb, KNIGHT | whosTurn))
			+ SCORE_BISHOP	* populationCount(getPieces(qb, BISHOP | whosTurn))
			+ SCORE_ROOK	* populationCount(getPieces(qb, ROOK   | whosTurn))
			+ SCORE_QUEEN	* populationCount(getPieces(qb, QUEEN  | whosTurn))
			
			- SCORE_PAWN 	* populationCount(getPieces(qb, PAWN   | opponent))
			- SCORE_KNIGHT	* populationCount(getPieces(qb, KNIGHT | opponent))
			- SCORE_BISHOP	* populationCount(getPieces(qb, BISHOP | opponent))
			- SCORE_ROOK	* populationCount(getPieces(qb, ROOK   | opponent))
			- SCORE_QUEEN	* populationCount(getPieces(qb, QUEEN  | opponent)) ;
			
}


//
// MOBILITY: A mildly expensive strategy for deciding the best move 
// 			 based on how many more possible PSUEDOLEGAL moves the player
//           has than it's opponent.
//

typedef bitboard (getterFuncPtr)(const quadboard, const byte); 
typedef bitboard (generatorFuncPtr)(const bitboard, const bitboard, const bitboard); 

scoreType countMoves(	const quadboard qb, 
						generatorFuncPtr generator, 
						const bitboard friends, 
						const bitboard enemies, 
						const byte pieceType) {
	
	scoreType subscore = 0;
	
	iterator piece = { 0ULL, getPieces(qb, pieceType) }; 
	piece = getNextItem(piece);
	
	while (piece.item) { 	
		
		// Add the number of moves this piece can make to the tally.
		//
		// NOTE: The call to generator probably blows our cache 
		//       right out of the water.
		subscore += (scoreType)populationCount(generator(piece.item, enemies, friends));
		
		piece = getNextItem(piece);
	}		
	
	return subscore;
}

scoreType evaluateMobility(const quadboard qb, const byte whosTurn, const byte opponent) {
	
	const bitboard friends = getTeamPieces(qb, whosTurn);
	const bitboard enemies = getTeamPieces(qb, opponent);
			
	return    countMoves(qb, generateKnightMoves, friends, enemies, KNIGHT | whosTurn)
			+ countMoves(qb, generateBishopMoves, friends, enemies, BISHOP | whosTurn)
			+ countMoves(qb, generateRookMoves,   friends, enemies, ROOK   | whosTurn)
			+ countMoves(qb, generateQueenMoves,  friends, enemies, QUEEN  | whosTurn)
			// For the very moment, skipping kings and pawns.
			
			- countMoves(qb, generateKnightMoves, friends, enemies, KNIGHT | opponent)
			- countMoves(qb, generateBishopMoves, friends, enemies, BISHOP | opponent)
			- countMoves(qb, generateRookMoves,   friends, enemies, ROOK   | opponent)
			- countMoves(qb, generateQueenMoves,  friends, enemies, QUEEN  | opponent);
			// For the very moment, skipping kings and pawns.

}

scoreType analyseLeafNonTerminal(const board* const b) {
	
	// Tell the world we still live.
	displaySpinningPulse();
		
	// We have hit the limit of our depth search - time to score the board.
	return (1 * evaluateMobility(b->quad, b->whosTurn, b->whosTurn ^ 1))
		 + (2 * evaluateMaterial(b->quad, b->whosTurn, b->whosTurn ^ 1));	

}



