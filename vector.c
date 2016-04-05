// This must be set by callers to singlePieceAttacks.
typedef struct {

	bitboard piece;
	bitboard softBlockers; 
	bitboard hardBlockers;
	
} attackContext;

// Used internally to this file.
typedef struct {

	bitboard vector;
	byte direction;
		
} directionalVector;


inline byte stillOnBoard(const bitboard cursor, const bitboard attack) {

	return 	!attack // Fell off the top or bottom of board. 
			|| 
			// This is the difference in the fileIx of the attack and cursor positions.
			// Normally it's 1 for sliding pieces. It can be 2 for Knights.
			//
			// However, if the attack wrapped around one of the vertical sides,
			// then the modulo distance will be 7. Can be 6 for Knights.
			//
			abs(getFile(attack) - getFile(cursor)) > 2;
}

inline bitboard applyVector(const directionalVector* const dc, const bitboard cursor) {

	// Create attack bitboard by shifting the piece
	// by the approprate vector offset.
	//
	return dc->direction ? cursor >> trailingBit_Bitboard(dc->vector)  // DOWN
						 : cursor << trailingBit_Bitboard(dc->vector); // UP	

}

bitboard applySlidingAttackVector(	const attackContext* const ac, 
									const directionalVector* const dc) {

	bitboard attacks = 0ULL;

	// All vectors are fixed and so need to be relative 
	// to a moving cursor, not the stationary piece.
	
	bitboard cursor = ac->piece;

	// Keep adding attacks along this vector
	// until the vector dies.
	do {

		// Get the next attack along this vector.
		bitboard attack = applyVector(dc, cursor);
									
		if (ac->hardBlockers & attack) {
			
			// We hit a friend, ignore the move and end the vector
			return attacks;
		}
		
		// Are we still on the board ?
		if (stillOnBoard(cursor, attack)) { 
			
			// We ran off the edge of the board, kill the vector.
			return attacks;
		}	
		else {

			// Add to the attack "list" (actually a bitboard)
			attacks |= attack;
			
			// Hitting a soft block means we still add the attack to the list,
			// but then kill the vector immediately after that.
			if (ac->softBlockers & attack) {
				
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

bitboard applySingleAttackVector(	const attackContext* const ac, 
									const directionalVector* const dc) {

	// Create an alias for now.
	bitboard cursor = ac->piece;

	// Get the next attack along this vector.
	bitboard attack = applyVector(dc, cursor);

	// Get the easiest thing to check out of the way first.		
	if (ac->hardBlockers & attack) {
		
		// We hit someone who prevents us adding this attack.
		// Return nothing.
		return 0ULL;
	}

	// Are we still on the board ?
	if (stillOnBoard(cursor, attack)) { 
		
		// We ran off the edge of the board, nothing to add.
		return 0ULL;
	}	
	else {
		
		// Single attacks don't need the concept of softBlockers.
		// Return the value, no need to signal end of vector because it's one shot only.
		return attack;
	}
	
}
