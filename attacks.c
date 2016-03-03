#define ATTACKMODE_SINGLE 	(0)
#define ATTACKMODE_SLIDING 	(1)
#define ATTACKMODE_PAWN 	(2)

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
		int moduloDistance = abs((trailingBit_Bitboard(attack) % 8) - (trailingBit_Bitboard(cursor) % 8));

		// Are we still on the board ?
		if (!attack || moduloDistance > 2) { // Anything larger than 2 is out.
											 // We say 2 to keep in line with
											 // single attack calculation.

			printf("We ran off the edge of the board, not adding attack to list.  Modulo distance %d.  Exiting sliding vector\n", moduloDistance);

			// We ran off the edge of the board, kill the vector.
			return attacks;
			
		}	
		else {

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
	int moduloDistance = abs((trailingBit_Bitboard(attack) % 8) - (trailingBit_Bitboard(cursor) % 8));

	// Are we still on the board ?
	if (!attack || moduloDistance > 2) { // Anything larger than 2 is out.
	
		// We ran off the edge of the board, nothing to add.
		return 0ULL;
		
	}	
	else {

		// Since we are in attack mode, just return this single attack
		// no matter what (as long as it's on the board, it's legit)
		return attack;

	}
	
}

bitboard pieceAttacks(bitboard pieces, bitboard softBlockers, bitboard positiveVectors, int attackMode) {

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
					attacks |= applySlidingAttackVector(piece.item, vector.item, softBlockers, dir);
					printf("DONE\n");
				}
				else {
					// This applies to SINGLE and PAWN attack modes.
					printf("Applying single attack vector...");
					attacks |= applySingleAttackVector(piece.item, vector.item, dir);
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

		
		// Get the next piece, if any.
		piece = getNextItem(piece);
		
	} // exits when pieces "list" is empty


	// Viola !
	return attacks;
		
}

bitboard generateAttacks(board* b, int team) {

	bitboard softBlockers = getFrenemies(b->quad);
	
	return
		// SLIDING PIECES
		  pieceAttacks(getQueens(b->quad, team),  softBlockers, nw | n | ne | w, ATTACKMODE_SLIDING)
		| pieceAttacks(getRooks(b->quad, team),   softBlockers,      n      | w, ATTACKMODE_SLIDING)
		| pieceAttacks(getBishops(b->quad, team), softBlockers, nw     | ne    , ATTACKMODE_SLIDING)
		
		// SINGLE AND PAWN PIECES
		| pieceAttacks(getKings(b->quad, team),   softBlockers, nw | n | ne | w, ATTACKMODE_SINGLE)
		| pieceAttacks(getPawns(b->quad, team),   softBlockers, nw     | ne    , ATTACKMODE_PAWN)
 		| pieceAttacks(getKnights(b->quad, team), softBlockers, nww | nnw |nne | nee, ATTACKMODE_SINGLE);
}

bitboard generateTestAttacks(board* b) {
	bitboard softBlockers = getFrenemies(b->quad);
	return pieceAttacks(1ULL << 35, softBlockers, nw|n|ne|w, ATTACKMODE_SLIDING);
}



