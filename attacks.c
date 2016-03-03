#define ATTACKMODE_SINGLE 	(0)
#define ATTACKMODE_SLIDING 	(1)
#define ATTACKMODE_PAWN 	(2)

#define DIRECTION_UP 	(0)
#define DIRECTION_DOWN	(1)

// These are positive-only attack vectors for all pieces except KNIGHT.
const bitboard nw = 1ULL << 9;
const bitboard n  = 1ULL << 8;
const bitboard ne = 1ULL << 7;
const bitboard w  = 1ULL << 1;

// These are positive-only attack vectors for KNIGHT.
const bitboard nww = 1ULL << 6;
const bitboard nnw = 1ULL << 15;
const bitboard nne = 1ULL << 17;
const bitboard nee = 1ULL << 10;

bitboard applySlidingAttackVector(	const bitboard piece, 
									const bitboard vector, 
									const bitboard softBlockers, 
									const bitboard hardBlockers, 
									const int direction) {

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
		int moduloDistance = abs(getFile(attack) - getFile(cursor));

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
									const int direction) {

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
	int moduloDistance = abs(getFile(attack) - getFile(cursor));

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

bitboard singlePieceAttacks(bitboard piece, bitboard softBlockers, bitboard hardBlockers, bitboard positiveVectors, int attackMode) {

	// This is the bitboard we build up and then return.
	bitboard attacks = 0ULL;

	// Since we only have positive vectors, do everything twice,
	// but the second time, do it all backwards.
	//
	// 0=UP, 1=DOWN
	//
	for (int dir = 0; dir < 2; dir ++) {

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

bitboard multiPieceAttacks(bitboard pieces, bitboard softBlockers, bitboard hardBlockers, bitboard positiveVectors, int attackMode) {

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
bitboard generateQueenMoves(bitboard piece, bitboard softBlockers, bitboard hardBlockers, int team) {
	return singlePieceAttacks(piece, softBlockers, hardBlockers, nw | n | ne | w, ATTACKMODE_SLIDING);
}
bitboard generateBishopMoves(bitboard piece, bitboard softBlockers, bitboard hardBlockers, int team) {
	return singlePieceAttacks(piece, softBlockers, hardBlockers,      n      | w, ATTACKMODE_SLIDING);
}
bitboard generateRookMoves(bitboard piece, bitboard softBlockers, bitboard hardBlockers, int team) {
	return singlePieceAttacks(piece, softBlockers, hardBlockers, nw     | ne    , ATTACKMODE_SLIDING);
}
bitboard generateKnightMoves(bitboard piece, bitboard softBlockers, bitboard hardBlockers, int team) {
	return singlePieceAttacks(piece, softBlockers, hardBlockers, nww | nnw |nne | nee, ATTACKMODE_SINGLE);
}
bitboard generateKingMoves(bitboard piece, bitboard softBlockers, bitboard hardBlockers, bitboard enemyCheckmap, int team) {
	return singlePieceAttacks(piece, softBlockers, hardBlockers, nww | nnw |nne | nee, ATTACKMODE_SINGLE);
	
	//
	// TODO: Add castling moves here.
	//
	
/*
 
 
				//
				// Make a list of all the castling options still available at this point.
				//
				int whiteQueenside = b->piecesMoved & (WHITE_QUEENSIDE_CASTLE_MOVED | WHITE_KING_MOVED);
				int blackQueenside = b->piecesMoved & (BLACK_QUEENSIDE_CASTLE_MOVED | BLACK_KING_MOVED);
				int whiteKingside = b->piecesMoved & (WHITE_KINGSIDE_CASTLE_MOVED | WHITE_KING_MOVED);
				int blackKingside = b->piecesMoved & (BLACK_KINGSIDE_CASTLE_MOVED | BLACK_KING_MOVED);

						
				if ((team == WHITE && whiteQueenside == 0) || (team == BLACK && blackQueenside == 0)) {

					//
					// Able to castle now, but first check for ugly stuff.
					//
					
					// Check that the intervening squares are clear of pieces.
					if (boardAt(b,1,y) == 0 && boardAt(b,2,y) == 0 && boardAt(b,3,y) == 0) {
						
						// Check that none of the squares from the castle to the king inclusive are checked.
						if (!isSquareRangeChecked(b, 0, 4, y, b->whosTurn)) {
							
							// makeMove (but not addMove) will recognise a King moving two squares and will take care of moving the castle for us.
							// The castle doesn't need to move when this move is being added to list of moves,
							// but will need to move should the move actually be played on a board.
							addAction(mvs,2,y);
						}
					}
					
				}
				
				if ((team == WHITE && whiteKingside == 0) || (team == BLACK && blackKingside == 0)) {

					//
					// Able to castle now, but first check for ugly stuff.
					//
										
					// Check that the intervening squares are clear of pieces.
					if (boardAt(b,5,y) == 0 && boardAt(b,6,y) == 0) {
					
						// Check that none of the squares from the king to the castle inclusive are checked.
						if (!isSquareRangeChecked(b, 4, 7, y, b->whosTurn)) {
							
							// makeMove (but not addMove) will recognise a King moving two squares and will take care of moving the castle for us.
							// The castle doesn't need to move when this move is being added to list of moves,
							// but will need to move should the move actually be played on a board.
							addAction(mvs,6,y);
						}
					}
				}
			}
 
  
 */	
	
}

//
// Generate a map of psuedolegal moves one piece can make - PAWN
//
bitboard generatePawnMoves(bitboard piece, bitboard softBlockers, bitboard hardBlockers, int team) {

	// Diagonal moves
	bitboard takingMoves = applySingleAttackVector(piece, ne, hardBlockers, DIRECTION_UP)
						   |
						   applySingleAttackVector(piece, nw, hardBlockers, DIRECTION_UP);

	// Only squares with enemy pieces on them can be moved into diagonally.
	takingMoves &= softBlockers;

	// 1 square move
	bitboard nonTakingMoves = applySingleAttackVector(piece, n, hardBlockers, DIRECTION_UP);

	// Pawns cannot "take" enemies when going straight forward.
	nonTakingMoves &= ~softBlockers;

	if (nonTakingMoves && (team == WHITE && piece < (1ULL << 16)) || (team == BLACK && piece > (1ULL << 55))) {

		// First move, and nothing hardBlocked OR softBlocked the 1 square move
		// - therefore can try to move two squares.	
		nonTakingMoves |= applySingleAttackVector(piece, n, hardBlockers, DIRECTION_UP);
		
		// Pawns cannot "take" enemies when going straight forward.
		nonTakingMoves &= ~softBlockers;
		
	}
	
	return takingMoves | nonTakingMoves;
}


bitboard generateTestCheckingMap(quadboard qb) {
	bitboard softBlockers = getFrenemies(qb);
	return multiPieceAttacks(1ULL << 35, softBlockers, 0ULL, nw|n|ne|w, ATTACKMODE_SLIDING);
}



