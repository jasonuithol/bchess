// ======================================================
//
// GENERIC PIECE IMPLEMENTATION
//
// ======================================================


bitboard pieceMoveGenerator(const attackContext* const ac, 
							const vectorsContext* const vc, 
							const board* const b	) {

								
	return singlePieceAttacks(ac, vc);
}


// ======================================================
//
// PAWN IMPLEMENTATION
//
// ======================================================

const vectorsContext pawnAheadContexts[2] = {
	
	{ applySingleAttackVector, n, 0, PAWN },
	{ applySingleAttackVector, 0, n, PAWN }
};

//
// After one generates pawn moves, one must call this method to see if extra information
// needs to be passed to spawnXXXBoard()
//
inline byte isPawnPromotable(const bitboard piece) {
	return (piece < (1ULL << 8) || piece > (1ULL << 55));
}

void pawnMoveAdder(	analysisList* const list, 
					const board* const old, 
					const analysisMove* const move,
					const byte leafMode	) {

	if (isPawnPromotable(move->from)) {
		// More snowflake pawn logic.
		addMoveIfLegal(list, old, &((analysisMove){move->from, move->to, QUEEN }), leafMode);
		addMoveIfLegal(list, old, &((analysisMove){move->from, move->to, BISHOP}), leafMode);
		addMoveIfLegal(list, old, &((analysisMove){move->from, move->to, ROOK  }), leafMode);
		addMoveIfLegal(list, old, &((analysisMove){move->from, move->to, KNIGHT}), leafMode);
	}
	else {
		// All other pieces included here.
		addMoveIfLegal(list, old, move, leafMode);
	}

}

//
// Generate a map of psuedolegal moves one piece can make - PAWN
//
bitboard pawnMoveGenerator(	const attackContext* const ac, 
							const vectorsContext* const vc, 
							const board* const b	) {

	// Diagonal moves. 
	// NOTE: Can only be allowed onto squares with enemies on them.
	const bitboard takingMoves = ac->softBlockers & singlePieceAttacks(ac, vc);

	// Forward moves (a bit trickier)
	const vectorsContext* moveVc = &(pawnAheadContexts[b->whosTurn]); 

	// 1 square move
	// NOTE: Pawns cannot "take" enemies when going straight forward.
	bitboard nonTakingMoves = ~ac->softBlockers & singlePieceAttacks(ac, moveVc);

	// Check to see if (a) the square ahead was clear and (b) our pawn is on it's original rank.
	if (nonTakingMoves && ((b->whosTurn == WHITE && ac->piece < (1ULL << 16)) || (b->whosTurn == BLACK && ac->piece > (1ULL << 47)))) {

		// Pretend we have a cursor
		const attackContext ac2 = {
			nonTakingMoves,
			ac->softBlockers,
			ac->hardBlockers
		};

		// First move, and nothing hardBlocked OR softBlocked the 1 square move
		// - therefore can try to move two squares.	
		// NOTE: Pawns cannot "take" enemies when going straight forward.
		nonTakingMoves |= (~ac->softBlockers & singlePieceAttacks(&ac2, moveVc));
	}
		
	return takingMoves | nonTakingMoves;
}


// ======================================================
//
// KING IMPLEMENTATION
//
// ======================================================

const vectorsContext kingSideCastleContext = {
	applySingleAttackVector,
	0,
	castle,
	KING	
};

const vectorsContext queenSideCastleContext = {
	applySingleAttackVector,
	castle,
	0,
	KING	
};

bitboard kingMoveGenerator(const attackContext* const ac, 
							const vectorsContext* const vc, 
							const board* const b	) {

	// First of all, the standard moves. 
	bitboard moves = singlePieceAttacks(ac, vc);

	if (b->currentCastlingRights) { // AN OPTIMIZATION FOR THE MOST COMMON SCENARIO

		//
		// Now check if there's any castling options available. 
		//
		if (b->whosTurn == WHITE) {
			
			// KINGSIDE CASTLING - WHITE
			if ((b->currentCastlingRights & (WHITE_KING_MOVED | WHITE_KINGSIDE_CASTLE_MOVED)) == 0) {
				moves |= singlePieceAttacks(ac, &(kingSideCastleContext));
			}

			// QUEENSIDE CASTLING - WHITE
			if ((b->currentCastlingRights & (WHITE_KING_MOVED | WHITE_QUEENSIDE_CASTLE_MOVED)) == 0) {
				moves |= singlePieceAttacks(ac, &(queenSideCastleContext));
			}

		}
		else {

			// KINGSIDE CASTLING - BLACK
			if ((b->currentCastlingRights & (BLACK_KING_MOVED | BLACK_KINGSIDE_CASTLE_MOVED)) == 0) {
				moves |= singlePieceAttacks(ac, &(kingSideCastleContext));
			}

			// QUEENSIDE CASTLING - BLACK
			if ((b->currentCastlingRights & (BLACK_KING_MOVED | BLACK_QUEENSIDE_CASTLE_MOVED)) == 0) {
				moves |= singlePieceAttacks(ac, &(queenSideCastleContext));
			}
			
		}
	}
	
	return moves;
}
