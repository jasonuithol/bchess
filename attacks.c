#define WHITE_QUEENSIDE_CASTLE_MOVED	(128)
#define WHITE_KING_MOVED				(64)
#define WHITE_KINGSIDE_CASTLE_MOVED		(32)
#define BLACK_QUEENSIDE_CASTLE_MOVED	(16)
#define BLACK_KING_MOVED				(8)
#define BLACK_KINGSIDE_CASTLE_MOVED		(4)

#define ATTACKMODE_SINGLE 	(0)
#define ATTACKMODE_SLIDING 	(1)
#define ATTACKMODE_PAWN 	(2)

//
// Note: 
//
// WHITE == UP   == 0
// BLACK == DOWN == 1
//
#define DIRECTION_UP 	(WHITE)
#define DIRECTION_DOWN	(BLACK)

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
		bitboard attack = direction ? cursor >> trailingBit_Bitboard(vector)  // DOWN
							        : cursor << trailingBit_Bitboard(vector); // UP

									
		if (hardBlockers & attack) {
			
			// We hit a friend, ignore the move and end the vector
			return attacks;
		}

	
		// This is the difference in the fileIx of the attack and cursor positions.
		// Normally it's 1 for sliding pieces.
		// However, if the attack wrapped around one of the vertical sides,
		// then the modulo distance will be 7.
		//
		// Are we still on the board ?
		if (!attack || abs(getFile(attack) - getFile(cursor)) > 2) { 
											 // Anything larger than 2 is out.
											 // We say 2 to keep in line with
											 // single attack calculation.

			// We ran off the edge of the board, kill the vector.
			return attacks;
			
		}	
		else {


			// Add to the attack "list" (actually a bitboard)
			attacks |= attack;
			
			// Hitting a soft block means we still add the attack to the list,
			// but then kill the vector immediately after that.
			if (softBlockers & attack) {
				
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
	bitboard attack = direction ? cursor >> trailingBit_Bitboard(vector)  // DOWN
						        : cursor << trailingBit_Bitboard(vector); // UP

	// Get the easiest thing to check out of the way first.
		
	if (hardBlockers & attack) {
		
		// We hit someone who prevents us adding this attack.
		// Return nothing.
		return 0ULL;
	}

	// This is the difference in the fileIx of the attack and cursor positions.
	// Normally it's 1 for kings, 1-2 for knights.
	// However, if the attack wrapped around one of the vertical sides,
	// then the modulo distance will be 6-7.
	//

	// Are we still on the board ?
	if (!attack || abs(getFile(attack) - getFile(cursor)) > 2) { // Anything larger than 2 is out.
		
		// We ran off the edge of the board, nothing to add.
		return 0ULL;
		
	}	
	else {
		
		// Single attacks don't need the concept of softBlockers.
		// Return the value, no need to signal end of vector because it's one shot only.
		return attack;
	}
	
}

bitboard singlePieceAttacks(const bitboard piece, const bitboard softBlockers, const bitboard hardBlockers, const bitboard positiveVectors, const byte attackMode) {


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
		
		// Iterating over the vectors.
		do { 

			// Grab all the attacks along this vector/dir combo.
			if (attackMode == ATTACKMODE_SLIDING) {
				attacks |= applySlidingAttackVector(piece, vector.item, softBlockers, hardBlockers, dir);
			}
			else {
				// This applies to SINGLE and PAWN attack modes.
				attacks |= applySingleAttackVector(piece, vector.item, hardBlockers, dir);
			}
			
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

byte isSquareAttacked(const quadboard qb, const bitboard square, const byte askingTeam) {

	const byte attackingTeam = askingTeam ^ 1;

	const bitboard enemies = getTeamPieces(qb, attackingTeam);
	const bitboard friends = getTeamPieces(qb, askingTeam);

	const bitboard enemyQueens = getPieces(qb, QUEEN | attackingTeam );
	const bitboard enemyBishops = getPieces(qb, BISHOP | attackingTeam);
	
	if ((enemyQueens|enemyBishops) & singlePieceAttacks(square, enemies, friends, bishopAttacks, ATTACKMODE_SLIDING)) {
		return 1;
	}

	const bitboard enemyRooks = getPieces(qb, ROOK | attackingTeam);

	if ((enemyQueens|enemyRooks) & singlePieceAttacks(square, enemies, friends, rookAttacks, ATTACKMODE_SLIDING)) {	
		return 1;
	}
	
	const bitboard enemyKnights = getPieces(qb, KNIGHT | attackingTeam);
	if (enemyKnights & singlePieceAttacks(square, enemies, friends, knightAttacks, ATTACKMODE_SINGLE)) {	
		return 1;
	}

	const bitboard enemyKings = getPieces(qb, KING | attackingTeam);
	if (enemyKings & singlePieceAttacks(square, enemies, friends, kingAttacks, ATTACKMODE_SINGLE)) {	
		return 1;
	}

	//
	// WARNING: FRAGILE CODE
	//
	// UP   is the direction WHITE pawns move.
	// DOWN is the direction BLACK pawns move.
	//
	// Therefore direction = team, because:
	//
	// WHITE == UP   == 0
	// BLACK == DOWN == 1
	//
	const byte direction = attackingTeam;

	const bitboard enemyPawns = getPieces(qb, PAWN | attackingTeam);
	if (enemyPawns & (applySingleAttackVector(square, ne, friends, direction)
				      |
					  applySingleAttackVector(square, nw, friends, direction))) {
						   	
		return 1;
	}

	return 0;
}

//
// Generate a map of psuedolegal moves one piece can make - EVERYTHING EXCEPT PAWNS
//


bitboard generateQueenMoves(const bitboard piece, const bitboard enemies, const bitboard friends) {
	return singlePieceAttacks(piece, enemies, friends, queenAttacks, ATTACKMODE_SLIDING);
}
bitboard generateBishopMoves(const bitboard piece, const bitboard enemies, const bitboard friends) {
	return singlePieceAttacks(piece, enemies, friends, bishopAttacks, ATTACKMODE_SLIDING);
}
bitboard generateRookMoves(const bitboard piece, const bitboard enemies, const bitboard friends) {
	return singlePieceAttacks(piece, enemies, friends, rookAttacks, ATTACKMODE_SLIDING);
}
bitboard generateKnightMoves(const bitboard piece, const bitboard enemies, const bitboard friends) {
	return singlePieceAttacks(piece, enemies, friends, knightAttacks, ATTACKMODE_SINGLE);
}

//
// Generate a map of psuedolegal moves one piece can make - KING
//
bitboard generateKingMoves(const bitboard piece, const bitboard enemies, const bitboard friends, const bitboard currentCastlingRights, const byte team) {

	// First of all, do the boring, ordinary 1 square moves.
	bitboard kingMoves = singlePieceAttacks(piece, enemies, friends, kingAttacks, ATTACKMODE_SINGLE);

	//
	// Now check if there's any castling options available. 
	//
	if (team == WHITE) {
		
		// KINGSIDE CASTLING - WHITE
		if (!(currentCastlingRights & WHITE_KINGSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends|enemies, DIRECTION_DOWN);
		}

		// QUEENSIDE CASTLING - WHITE
		if (!(currentCastlingRights & WHITE_QUEENSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends|enemies, DIRECTION_UP);
		}

	}
	else {

		// KINGSIDE CASTLING - BLACK
		if (!(currentCastlingRights & BLACK_KINGSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends|enemies, DIRECTION_UP);
		}

		// QUEENSIDE CASTLING - BLACK
		if (!(currentCastlingRights & BLACK_QUEENSIDE_CASTLE_MOVED)) {
			kingMoves |= applySingleAttackVector(piece, 2ULL, friends|enemies, DIRECTION_DOWN);
		}
		
	}
	
	return kingMoves;
}


//
// Generate a map of psuedolegal moves one piece can make - PAWN
//
bitboard generatePawnMoves(const bitboard piece, const bitboard enemies, const bitboard friends, const byte team) {


	//
	// WARNING: FRAGILE CODE
	//
	// UP   is the direction WHITE pawns move.
	// DOWN is the direction BLACK pawns move.
	//
	// Therefore direction = team, because:
	//
	// WHITE == UP   == 0
	// BLACK == DOWN == 1
	//
	const byte direction = team;



	// Diagonal moves. We skip the wrapper methods which do zero checking
	// and optimise by directly calling the target method.
	//
	// It is recommended to do all pawn moves last so that CPU branch prediction
	// has an easier time of things.
	//
	bitboard takingMoves = applySingleAttackVector(piece, ne, friends, direction)
						   |
						   applySingleAttackVector(piece, nw, friends, direction);


	// Only squares with enemy pieces on them can be moved into diagonally.
	takingMoves &= enemies;

	// 1 square move
	bitboard nonTakingMoves = applySingleAttackVector(piece, n, friends, direction);

	// Pawns cannot "take" enemies when going straight forward.
	nonTakingMoves &= ~enemies;

	// Check to see if our pawn is on it's original rank.
	if (nonTakingMoves && ((team == WHITE && piece < (1ULL << 16)) || (team == BLACK && piece > (1ULL << 47)))) {

		// First move, and nothing hardBlocked OR softBlocked the 1 square move
		// - therefore can try to move two squares.	
		nonTakingMoves |= applySingleAttackVector(nonTakingMoves, n, friends, direction);
		
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




