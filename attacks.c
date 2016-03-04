#define WHITE_QUEENSIDE_CASTLE_MOVED	128
#define WHITE_KING_MOVED				64
#define WHITE_KINGSIDE_CASTLE_MOVED		32
#define BLACK_QUEENSIDE_CASTLE_MOVED	16
#define BLACK_KING_MOVED				8
#define BLACK_KINGSIDE_CASTLE_MOVED		4

#define ATTACKMODE_SINGLE 	(0)
#define ATTACKMODE_SLIDING 	(1)
#define ATTACKMODE_PAWN 	(2)

#define DIRECTION_UP 	(0)
#define DIRECTION_DOWN	(1)

// These are positive-only attack vectors for all pieces except KNIGHT.
#define nw (1ULL << 9)
#define n  (1ULL << 8)
#define ne (1ULL << 7)
#define w  (1ULL << 1)

// These are positive-only attack vectors for KNIGHT.
#define nww (1ULL << 6)
#define nnw (1ULL << 15)
#define nne (1ULL << 17)
#define nee (1ULL << 10)

const bitboard queenAttacks 	= nw | n | ne | w;
const bitboard bishopAttacks 	= nw | ne;
const bitboard rookAttacks 		= n  | w;    
const bitboard kingAttacks 		= nw | n | ne | w;
const bitboard knightAttacks	= nww | nnw |nne | nee;
// Pawns are special.  Verrrry special.


//
// For generic generateXXXMoves() calls.  
//
// NOTE: generateKingMoves does not comply, because it takes the parameter castlingCheckingMap
//
typedef bitboard (*generatorFuncPtr)(bitboard piece, bitboard enemies, bitboard friends, byte team);



bitboard applySlidingAttackVector(	const bitboard piece, 
									const bitboard vector, 
									const bitboard softBlockers, 
									const bitboard hardBlockers, 
									const byte direction) {

	bitboard attacks = 0ULL;

	// All vectors are fixed and so need to be relative 
	// to a moving cursor, not the stationary piece.
	
	bitboard cursor = piece;

	// Keep adding attacks along this vector
	// until the vector dies.
	do {

		// Create attack bitboard by shifting the piece
		// by the approprate vector offset.
		//
		printf("Applying sliding attack vector to cursor in direction %s, printing cursor and vector bitboards\n", direction ? "DOWN" : "UP");
		printBB(cursor);
		printf("\n");
		printBB(vector);
		
		bitboard attack = direction ? cursor >> trailingBit_Bitboard(vector)  // DOWN
							        : cursor << trailingBit_Bitboard(vector); // UP
						  

		printf("Showing calculated attack destination\n");
		printBB(attack);
		
		// DEBUG ONLY
		if (attack == 0) {
			printf("Obtained zero from 0x%" PRIx64 " << %u\n", cursor, trailingBit_Bitboard(vector));
		}
									
		// This is the difference in the fileIx of the attack and cursor positions.
		// Normally it's 1 for sliding pieces.
		// However, if the attack wrapped around one of the vertical sides,
		// then the modulo distance will be 7.
		//
		offset moduloDistance = abs(getFile(attack) - getFile(cursor));

		// Are we still on the board ?
		if (!attack || moduloDistance > 2) { // Anything larger than 2 is out.
											 // We say 2 to keep in line with
											 // single attack calculation.

			printf("We ran off the edge of the board, not adding attack to list.  Modulo distance %d.  Exiting sliding vector\n", moduloDistance);

			// We ran off the edge of the board, kill the vector.
			return attacks;
			
		}	
		else {

			if (hardBlockers & attack) {

				printf("We ran into a hard blocker, not adding attack to list.  Exiting sliding vector\n");
				
				// We hit someone, end the vector
				return attacks;
			}

			// Add to the attack "list" (actually a bitboard)
			attacks |= attack;
			
			// Hitting a soft block means we still add the attack to the list,
			// but then kill the vector immediately after that.
			if (softBlockers & attack) {

				printf("We ran into a soft blocker, still adding attack to list.  Exiting sliding vector\n");
				
				// We hit someone, end the vector
				return attacks;
			}
			
			// Move the cursor
			cursor = attack;
		}
	
	} while (1); // I hate having this here, 
				 // but anything else adds 
				 // overhead in a critical loop.

	//
	// Tell the compiler that this bit is UNREACHABLE
	//
	__builtin_unreachable();
}

bitboard applySingleAttackVector(	const bitboard cursor,
									const bitboard vector, 
									const bitboard hardBlockers, 
									const byte direction) {

	// Create attack bitboard by shifting the piece
	// by the approprate vector offset.
	//
	printf("Applying single attack vector to cursor in direction %d (0=UP,1=DOWN), printing piece bitboard\n", direction);
	printBB(cursor);
	
	bitboard attack = direction ? cursor >> trailingBit_Bitboard(vector)  // DOWN
						        : cursor << trailingBit_Bitboard(vector); // UP

					  
	printf("Showing calculated attack destination\n");
	printBB(attack);

	// This is the difference in the fileIx of the attack and cursor positions.
	// Normally it's 1 for kings, 1-2 for knights.
	// However, if the attack wrapped around one of the vertical sides,
	// then the modulo distance will be 6-7.
	//
	offset moduloDistance = abs(getFile(attack) - getFile(cursor));

	// Are we still on the board ?
	if (!attack || moduloDistance > 2) { // Anything larger than 2 is out.
	
		// We ran off the edge of the board, nothing to add.
		return 0ULL;
		
	}	
	else {

		if (hardBlockers & attack) {

			printf("We ran into a hard blocker, not adding attack to list.  Exiting sliding vector\n");
			
			// We hit someone who prevents us adding this attack.
			// Return nothing.
			return 0ULL;
		}
		else {
			
			// Single attacks don't need the concept of softBlockers.
			// Return the value, no need to signal end of vector because it's one shot only.
			return attack;
		}
	}
	
}

bitboard singlePieceAttacks(bitboard piece, bitboard softBlockers, bitboard hardBlockers, bitboard positiveVectors, byte attackMode) {

	// This is the bitboard we build up and then return.
	bitboard attacks = 0ULL;

	// Since we only have positive vectors, do everything twice,
	// but the second time, do it all backwards.
	//
	// 0=UP, 1=DOWN
	//
	for (byte dir = 0; dir < 2; dir ++) {

		// Create a new vector scratchlist.
		iterator vector = { 0ULL, positiveVectors };
		vector = getNextItem(vector);
		
		printf("Pulled new vector off vectorList, showing vector bitboard\n");
		printBB(vector.item);

		// Iterating over the vectors.
		do { 

			// Grab all the attacks along this vector/dir combo.
			if (attackMode == ATTACKMODE_SLIDING) {
				printf("Applying sliding attack vector...");
				attacks |= applySlidingAttackVector(piece, vector.item, softBlockers, hardBlockers, dir);
				printf("DONE\n");
			}
			else {
				// This applies to SINGLE and PAWN attack modes.
				printf("Applying single attack vector...");
				attacks |= applySingleAttackVector(piece, vector.item, hardBlockers, dir);
				printf("DONE\n");
			}
			
			printf("Showing accumulated attacks bitboard\n");
			printBB(attacks);
			
			
			// Fetch the next vector (if any left).
			vector = getNextItem(vector);
		
		// Have we run out of vectors?
		} while (vector.item);

		// Don't do the backwards direction for pawns.
		if (attackMode == ATTACKMODE_PAWN) {
			break;
		}

	} // The forwards/backwards direction loop ends here

	// Viola !
	return attacks;
}

bitboard multiPieceAttacks(bitboard pieces, bitboard softBlockers, bitboard hardBlockers, bitboard positiveVectors, byte attackMode) {

	// This is the bitboard we build up and then return.
	bitboard attacks = 0ULL;

	//
	// Used to set initial value of cursor for each vector path
	// AND to remove the piece from the piece list in the piece loop.
	//	
	iterator piece = { 0ULL, pieces };
	piece = getNextItem(piece);

	// There might be zero pieces, so check up front.
	while (piece.item) { 

		printf("Entering new piece loop iteration, showing piece bitboard\n");
		printBB(piece.item);

		// Add all the attacks by this piece.
		attacks |= singlePieceAttacks(piece.item, softBlockers, hardBlockers, positiveVectors, attackMode);
		
		// Get the next piece, if any.
		piece = getNextItem(piece);
		
	} // exits when pieces "list" is empty


	// Viola !
	return attacks;
		
}



//
// Generate a map of psuedolegal moves one piece can make - EVERYTHING EXCEPT PAWNS
//


bitboard generateQueenMoves(bitboard piece, bitboard enemies, bitboard friends, byte team) {
	return singlePieceAttacks(piece, enemies, friends, queenAttacks, ATTACKMODE_SLIDING);
}
bitboard generateBishopMoves(bitboard piece, bitboard enemies, bitboard friends, byte team) {
	return singlePieceAttacks(piece, enemies, friends, bishopAttacks, ATTACKMODE_SLIDING);
}
bitboard generateRookMoves(bitboard piece, bitboard enemies, bitboard friends, byte team) {
	return singlePieceAttacks(piece, enemies, friends, rookAttacks, ATTACKMODE_SLIDING);
}
bitboard generateKnightMoves(bitboard piece, bitboard enemies, bitboard friends, byte team) {
	return singlePieceAttacks(piece, enemies, friends, knightAttacks, ATTACKMODE_SINGLE);
}

//
// Generate a map of psuedolegal moves one piece can make - KING
//
bitboard generateKingMoves(bitboard piece, bitboard enemies, bitboard friends, bitboard castlingCheckingMap, byte piecesMoved, byte team) {

	//
	// Yes, nested functions.  I'm hoping the optimizer will respect
	// that they're private and hence optimizable to heck and beyond.
	//
	byte castlingSquaresClear(bitboard castlingSquares) {
		return !((enemies|friends|castlingCheckingMap) & castlingSquares);
	}

	byte whiteCanCastle(byte castleMoveFlag) {
		return !(piecesMoved & (WHITE_KING_MOVED | castleMoveFlag));
	}

	byte blackCanCastle(byte castleMoveFlag) {
		return !(piecesMoved & (BLACK_KING_MOVED | castleMoveFlag));
	}

	// -----------------------------------------------------------------
	//
	// Outer method BEGINS HERE
	//

	// First of all, do the boring, ordinary 1 square moves.
	bitboard kingMoves = singlePieceAttacks(piece, enemies, friends, kingAttacks, ATTACKMODE_SINGLE);

	//
	// Now check if there's any castling options available. 
	//
	if (team == WHITE) {
		
		// KINGSIDE CASTLING - WHITE
		if (castlingSquaresClear(15ULL) && whiteCanCastle(WHITE_KINGSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends, DIRECTION_DOWN);
		}

		// QUEENSIDE CASTLING - WHITE
		if (castlingSquaresClear(31ULL << 3) && whiteCanCastle(WHITE_QUEENSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends, DIRECTION_UP);
		}

	}
	else {

		// KINGSIDE CASTLING - BLACK
		if (castlingSquaresClear(15ULL) && blackCanCastle(BLACK_KINGSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends, DIRECTION_UP);
		}

		// QUEENSIDE CASTLING - BLACK
		if (castlingSquaresClear(31ULL << 3) && whiteCanCastle(BLACK_QUEENSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends, DIRECTION_DOWN);
		}
		
	}
	
	return kingMoves;
}


//
// Generate a map of psuedolegal moves one piece can make - PAWN
//
bitboard generatePawnMoves(bitboard piece, bitboard enemies, bitboard friends, byte team) {

	// Diagonal moves. We skip the wrapper methods which do zero checking
	// and optimise by directly calling the target method.
	//
	// It is recommended to do all pawn moves last so that CPU branch prediction
	// has an easier time of things.
	//
	bitboard takingMoves = applySingleAttackVector(piece, ne, friends, DIRECTION_UP)
						   |
						   applySingleAttackVector(piece, nw, friends, DIRECTION_UP);

	// Only squares with enemy pieces on them can be moved into diagonally.
	takingMoves &= enemies;

	// 1 square move
	bitboard nonTakingMoves = applySingleAttackVector(piece, n, friends, DIRECTION_UP);

	// Pawns cannot "take" enemies when going straight forward.
	nonTakingMoves &= ~enemies;

	// Check to see if our pawn is on it's original rank.
	if (nonTakingMoves && (team == WHITE && piece < (1ULL << 16)) || (team == BLACK && piece > (1ULL << 47))) {

		// First move, and nothing hardBlocked OR softBlocked the 1 square move
		// - therefore can try to move two squares.	
		nonTakingMoves |= applySingleAttackVector(piece, n, friends, DIRECTION_UP);
		
		// Pawns cannot "take" enemies when going straight forward.
		nonTakingMoves &= ~enemies;
		
	}
	
	//
	// NOTE: Unlike the old version, the pawn move generator cares nothing for pawn promotion.
	//       As far as it knows, the pawn hits the back rank and that's the end of it.
	//
	//       The AI and human ask the umpire if a pawn promotion is needed.
	//
	
	return takingMoves | nonTakingMoves;
}

//
// Returns a map of all squares in check.
//
bitboard generateCheckingMap(quadboard qb, byte team) {

	// Everyone is a Soft Blocker when generating a checking map.
	const bitboard frenemies = getFrenemies(qb);
	
	return
		// SLIDING PIECES
		  multiPieceAttacks(getQueens(qb, team),  frenemies, 0ULL, queenAttacks, ATTACKMODE_SLIDING)
		| multiPieceAttacks(getRooks(qb, team),   frenemies, 0ULL, rookAttacks, ATTACKMODE_SLIDING)
		| multiPieceAttacks(getBishops(qb, team), frenemies, 0ULL, bishopAttacks, ATTACKMODE_SLIDING)
		
		// SINGLE AND PAWN PIECES
		| multiPieceAttacks(getKings(qb, team),   frenemies, 0ULL, kingAttacks, ATTACKMODE_SINGLE)
		| multiPieceAttacks(getPawns(qb, team),   frenemies, 0ULL, nw | ne, ATTACKMODE_PAWN)
 		| multiPieceAttacks(getKnights(qb, team), frenemies, 0ULL, knightAttacks, ATTACKMODE_SINGLE);
}

bitboard generateTestCheckingMap(quadboard qb) {
	bitboard softBlockers = getFrenemies(qb);
	return multiPieceAttacks(1ULL << 35, softBlockers, 0ULL, nw|n|ne|w, ATTACKMODE_SLIDING);
}







