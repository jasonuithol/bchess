
#define SCORE_PAWN 			(1)
#define SCORE_KNIGHT 		(4)
#define SCORE_BISHOP 		(6)
#define SCORE_ROOK 			(14)
#define SCORE_QUEEN 		(21)
//#define SCORE_KING_CHECK 	(10)

typedef uint32_t nodesCalculatedType;
typedef uint8_t depthType;



// GLOBAL VARIABLE
nodesCalculatedType nodesCalculated;




//
// Animates a little spinner to show that we are still alive.
//
// IMPORTANT: Do not use print() !! Instead, use printf() 
// We don't want this going into the logs !!!!
//

#define PULSE_SPIN_MAGNITUDE (17)

void displaySpinningPulse() {

	nodesCalculated = (nodesCalculated + 1) 
					  & 
					  (nodesCalculatedType)((1ULL < (PULSE_SPIN_MAGNITUDE + 1)) - 1); 

	switch (nodesCalculated) {
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
// A strategy for deciding the best move based on how much material the player has at the end of each possible move.
//

scoreType evaluateMaterial(const quadboard qb, const byte team) {

	return    SCORE_PAWN 	* populationCount(getPawns(qb, team))
			+ SCORE_KNIGHT	* populationCount(getKnights(qb, team))
			+ SCORE_BISHOP	* populationCount(getBishops(qb, team))
			+ SCORE_ROOK	* populationCount(getRooks(qb, team))
			+ SCORE_QUEEN	* populationCount(getQueens(qb, team))
			
			- SCORE_PAWN 	* populationCount(getPawns(qb, 1 - team))
			- SCORE_KNIGHT	* populationCount(getKnights(qb, 1 - team))
			- SCORE_BISHOP	* populationCount(getBishops(qb, 1 - team))
			- SCORE_ROOK	* populationCount(getRooks(qb, 1 - team))
			- SCORE_QUEEN	* populationCount(getQueens(qb, 1 - team)) ;
			
}


//
// MOBILITY: A strategy for deciding the best move based on how many 
//           more possible moves the player has than it's opponent.
//
scoreType scorePieceType(getterFuncPtr getter, generatorFuncPtr generator, bitboard friends, bitboard enemies, byte team) {
	
	scoreType subscore = 0;
	
	iterator piece = { 0ULL, getter(qb, team) }; 
	piece = getNextItem(piece);
	
	while (piece.item) { 	
		
		// Add the number of moves this piece can make to the tally.
		subscore += (scoreType)populationCount(generator(piece.item, enemies, friends, team));
		
		piece = getNextItem(piece);
	}		
	
	return subscore;
}

scoreType evaluateMobility(const quadboard qb, const byte team) {
	
	bitboard friends = getFriends(qb, team);
	bitboard enemies = getEnemies(qb, team);
		
	return    scorePieceType(getPawns,   generatePawnMoves,   friends, enemies, team)
			+ scorePieceType(getKnights, generateKnightMoves, friends, enemies, team)
			+ scorePieceType(getBishops, generateBishopMoves, friends, enemies, team)
			+ scorePieceType(getRooks,   generateRookMoves,   friends, enemies, team)
			+ scorePieceType(getQueens,  generateQueenMoves,  friends, enemies, team)
			// For the very moment, skipping kings.
			
			- scorePieceType(getPawns,   generatePawnMoves,   friends, enemies, 1 - team)
			- scorePieceType(getKnights, generateKnightMoves, friends, enemies, 1 - team)
			- scorePieceType(getBishops, generateBishopMoves, friends, enemies, 1 - team)
			- scorePieceType(getRooks,   generateRookMoves,   friends, enemies, 1 - team)
			- scorePieceType(getQueens,  generateQueenMoves,  friends, enemies, 1 - team);
			// For the very moment, skipping kings.
}


scoreType analyseLeafNonTerminal(quadboard qb, byte team) {
	
	// Tell the world we still live.
	displaySpinningPulse();

	// We have hit the limit of our depth search - time to score the board.
	return (1 * evaluateMobility(qb, team))
		 + (2 * evaluateMaterial(qb, team));	
					  
}

scoreType analyseLeafTerminal(board* b, byte scoringTeam, depthType depth) {

	if (b->whosTurn == scoringTeam) {
		
		byte boardState = detectCheckmate(b);
		
		if (boardState == BOARD_CHECKMATE) {
			logg("Detected possible checkmate defeat\n");
			return -9998 + (depth);
		}
		else {
			logg("Detected possible stalemate\n");
			return 0;
		}
	}
	else {
		
		byte boardState = detectCheckmate(b);
		
		if (boardState == BOARD_CHECKMATE) {
			logg("Detected possible checkmate victory\n");
			return 9998 - (depth);
		}
		else {
			logg("Detected possible stalemate\n");
			return 0;
		}
	}
	
}


