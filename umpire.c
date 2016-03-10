#define BOARD_LEGAL 	(1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL 	(0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)


typedef struct { // 35 bytes
	quadboard quad;
	byte currentCastlingRights; 	// Used to check castling ability for CURRENT move only.
	byte piecesMoved;  				// Used to check castling ability for all future moves.
	byte whosTurn;     				// 0 = WHITE, 1 = BLACK.
} board;

typedef int16_t scoreType;

typedef struct {

	bitboard from;
	bitboard to;
	scoreType score;
	byte promoteTo;
	board resultingBoard;
	
} analysisMove;


#define ANALYSIS_SIZE (255)

typedef struct {
	
	analysisMove items[ANALYSIS_SIZE];
	byte ix;
	
} analysisList; 


void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
}

// After one generates pawn moves, one must call this method to see if extra information
// needs to be passed to spawnXXXBoard()
byte isPawnPromotable(const bitboard piece) {
	return (piece < (1ULL << 8) || piece > (1ULL << 55));
}

byte isKingChecked(const quadboard qb, byte team) {
	return isSquareAttacked(qb, getPieces(qb, KING | team), team);
}

//
// PRECONDITION: Only call this if there were no legal moves to make.
//
byte determineEndOfGameState(const board* const b) {
	return isKingChecked(b->quad, b->whosTurn)
		   ? BOARD_CHECKMATE
		   : BOARD_STALEMATE;
}

//
// All moves MUST be performed by this method to ensure that:
//
// * Castling is managed properly.
// * Pawns get promoted.
// * Illegal board positions (involving check) are thrown out.
//
byte spawnLeafBoard(const board* const old, 
					board* const new, 
					const bitboard from, 
					const bitboard to, 
					const byte promoteTo) {


	quadboard* const qb = &(new->quad);

	// To create a new board from a current board, first copy the current board's content to the new one.
	memcpy((void*)new, (void*)old, sizeof(board));

	// Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
	moveSquare(qb, from, to);

	// Change the turn on the new board.  To get the team that just moved, use old->whosTurn.
	new->whosTurn = old->whosTurn ^ 1;


	// Now we can perform the legality check
	//
	if (isSquareAttacked(new->quad, getPieces(new->quad, KING | old->whosTurn), old->whosTurn)) {
		return BOARD_NOT_LEGAL;
	}

	
	//
	// Pawn promotion followup (takes the chosen promotion and applies it)
	//
	if (promoteTo > 0) {
		resetSquares(qb, to);
		addPieces(qb, to, promoteTo | old->whosTurn);
	}

	//
	// Castling followup (moves the castle over the king).
	//
	if (getPieces(old->quad, KING | old->whosTurn) & from) {
		if (getFile(from) - getFile(to) == 2) {
			// Kingside castle detected
			moveSquare(qb, 1ULL, 4ULL);
		}
		else if (getFile(from) - getFile(to) == -2) {
			// Queenside castle detected
			moveSquare(qb, 128ULL, 16ULL);
		}
	}


	// Board passed the illegal check state test earlier, so board is legal.
	return BOARD_LEGAL;
}


//
// When an AI or human chooses a move to play, use this method.
// If an AI is pondering a move higher than leaf level, also use this method.
//
// Since this board is going to have future moves made against it, maintain a bit more
// state.  It's more expensive, but required for boards that actually get played on.
//
// The vast, vast majority of leaf boards never get spawnXXXBoard called on them,
// only the Chosen Ones do. Use spawnLeafBoard for leaf boards.
//
byte spawnFullBoard(const board* const old, 
					board* const new, 
					const bitboard from, 
					const bitboard to, 
					const byte promoteTo) {


	// First, spawn a leaf board, it will have all the tidying up and illegal position checks.
	if (spawnLeafBoard(old, new, from, to, promoteTo) == BOARD_NOT_LEGAL) {
		
		// Board was found to be illegal, abort and notify caller.
		return BOARD_NOT_LEGAL;
	}
	
	//
	// To see if it's still possible to castle in the future, 
	// we track whether the relevant pieces have moved.
	//
	switch(from) {
		case 0: 			new->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; 	break;
		case 4: 			new->piecesMoved |= WHITE_KING_MOVED; 				break;
		case 7: 			new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED; 	break;
		case 7 * 256 + 0: 	new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; 	break;
		case 7 * 256 + 4: 	new->piecesMoved |= BLACK_KING_MOVED; 				break;
		case 7 * 256 + 7: 	new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED; 	break;
	}

	const bitboard enemies = getTeamPieces(new->quad, old->whosTurn);
	const bitboard friends = getTeamPieces(new->quad, new->whosTurn);

	//
	// We need this for checking to see if in the next move, a castling move is allowed
	//
	// NOTE: FOR LEAF BOARDS, THIS IS A NEEDLESS COST !!!!!
	//
	new->currentCastlingRights = 0;
	if (new->whosTurn == WHITE) {
		if (!(new->piecesMoved & (WHITE_KING_MOVED|WHITE_KINGSIDE_CASTLE_MOVED)) && !(enemies|friends|15ULL)) {
			for (byte i = 0; i < 4; i++) {
				if (isSquareAttacked(new->quad, 1ULL << i, new->whosTurn)) {
					// The thinking goes that if one square is attacked, 
					// then they all may as well be - there's no difference.
					new->currentCastlingRights = WHITE_KINGSIDE_CASTLE_MOVED;
				}
			}
		}
		if (!(new->piecesMoved & (WHITE_KING_MOVED|WHITE_QUEENSIDE_CASTLE_MOVED)) && !(enemies|friends|31ULL << 3)) {
			for (byte i = 5; i < 9; i++) {
				if (isSquareAttacked(new->quad, 1ULL << i, new->whosTurn)) {
					// The thinking goes that if one square is attacked, 
					// then they all may as well be - there's no difference.
					new->currentCastlingRights |= WHITE_QUEENSIDE_CASTLE_MOVED;
				}
			}
		}
	}
	else {
		if (!(new->piecesMoved & (BLACK_KING_MOVED|BLACK_KINGSIDE_CASTLE_MOVED)) && !(enemies|friends|15ULL)) {
			for (byte i = 0; i < 4; i++) {
				if (isSquareAttacked(new->quad, 1ULL << (i + 56), new->whosTurn)) {
					// The thinking goes that if one square is attacked, 
					// then they all may as well be - there's no difference.
					new->currentCastlingRights = BLACK_KINGSIDE_CASTLE_MOVED;
				}
			}
		}
		if (!(new->piecesMoved & (BLACK_KING_MOVED|BLACK_QUEENSIDE_CASTLE_MOVED)) && !(enemies|friends|31ULL << 3)) {
			for (byte i = 5; i < 9; i++) {
				if (isSquareAttacked(new->quad, 1ULL << (i + 56), new->whosTurn)) {
					// The thinking goes that if one square is attacked, 
					// then they all may as well be - there's no difference.
					new->currentCastlingRights |= BLACK_QUEENSIDE_CASTLE_MOVED;
				}
			}
		}	
	}

	// Board passed the illegal check state test earlier, so board is legal.
	return BOARD_LEGAL;

}

byte addMoveIfLegal(	analysisList* const list, 
						const board* const old, 
						const bitboard from, 
						const bitboard to, 
						const byte promoteTo, 
						const byte leafMode) {

	if (list->ix < ANALYSIS_SIZE) {
		
		analysisMove* const next = &(list->items[list->ix]);
		
		next->from      = from;
		next->to        = to;
		next->score     = 0;
		next->promoteTo = promoteTo;	   
		
		//
		// There's only one way to check if a move is legal and that's to
		// spawn the board, generate a checking map and look at the results.  
		// So, if we are going to all that trouble, then we may as well cache
		// the board.
		//
		// Especially if spawnFullBoard is called - because the very next
		// thing that will happen (once this loop is done) is that the 
		// board will get moves generated against it.
		//
		// The trade-off is space.  Board is currently 42 bytes.
		// A move can be compressed to 1 byte.  But now it weighs in
		// at a hefty 61 bytes (analysisMove) !!!!
		//
		// All this caching of state might get thrown out the window.
		// It's gotten way out of hand.
		//
		// The prefetcher won't be able to stream the resulting array
		// because the operations on it are quite complex:
		//
		// bestMove pt1 -> generateMoveList -> getPiece -> generateAttacks 
		// -> all that vector stuff -> YOU ARE HERE -> spawnXXXBoard
		// -> generateCheckingMap -> getpiece -> generateAttacks 
		// -> all that vector stuff
		// 
		// If FULL board: generateCheckingMap (again, but for the other side)
		// -> getPiece -> generateAttacks -> all that vector stuff
		//
		// IF leaf level: -> leafAnalysis -> material -> mobility 
		// -> generateMoveList ???? -> getPiece -> generateAttacks
		// -> all that vector stuff -> RETURN 
		//
		// ELSE bestMove pt1 recursion
		//
		// AFTER RECURSION (inside bestmove) -> min max -> RETURN
		//
		//
		const byte legality = leafMode 
							? spawnLeafBoard(old, &(next->resultingBoard), from, to, promoteTo)
							: spawnFullBoard(old, &(next->resultingBoard), from, to, promoteTo);

		if (legality == BOARD_LEGAL) {
//			print("Keeping move\n");
			// Keep this board.
			list->ix++;
		}
//		else {
//			print("Discarding move\n");
//		}
		
		//
		// Tell caller what happened re legality 
		// (illegal implies that move was thrown out)
		//
		return legality;
	}
	else {
		error("\nMaximum analysis moves size exceeded.\n");
	}

}

void generateLegalMoveList(const board* const b, analysisList* const moveList, const byte leafMode) {
	
	const bitboard friends = getTeamPieces(b->quad, b->whosTurn);
	const bitboard enemies = getTeamPieces(b->quad, b->whosTurn ^ 1);

	// Excludes King
	for (byte pieceType = PAWN; pieceType < KING; pieceType += 2) {

		iterator piece = { 0ULL, getPieces(b->quad, pieceType | b->whosTurn) };
		piece = getNextItem(piece);
			
		while (piece.item) {
			
			bitboard moves = 0ULL;
			
			switch(pieceType) {
				case PAWN: 	 moves = generatePawnMoves(piece.item, enemies, friends, b->whosTurn);    break;
				case ROOK:   moves = generateRookMoves(piece.item, enemies, friends);    break;
				case KNIGHT: moves = generateKnightMoves(piece.item, enemies, friends);  break;
				case BISHOP: moves = generateBishopMoves(piece.item, enemies, friends);  break;
				case QUEEN:  moves = generateQueenMoves(piece.item, enemies, friends);   break;
				default: error("Bad pieceType\n");
			}
			
//			print("Generated %d moves for pieceType %u\n", populationCount(moves), pieceType);
			
			iterator move = { 0ULL, moves };
			move = getNextItem(move);

			while (move.item) {
				
				if (pieceType == PAWN && isPawnPromotable(move.item)) {
					addMoveIfLegal(moveList, b, piece.item, move.item, QUEEN, leafMode);
					addMoveIfLegal(moveList, b, piece.item, move.item, KNIGHT, leafMode);
					addMoveIfLegal(moveList, b, piece.item, move.item, BISHOP, leafMode);
					addMoveIfLegal(moveList, b, piece.item, move.item, ROOK, leafMode);
				}
				else {
					addMoveIfLegal(moveList, b, piece.item, move.item, 0, leafMode);
				}
				
				move = getNextItem(move);
			}
			
			piece = getNextItem(piece);
		}

	}

	//
	// King is a special case
	//
	{
		const bitboard king = getPieces(b->quad, KING | b->whosTurn);
		const bitboard moves = generateKingMoves(king, enemies, friends, b->currentCastlingRights, b->whosTurn);

		iterator move = { 0ULL, moves };
		move = getNextItem(move);

		while (move.item) {
			addMoveIfLegal(moveList, b, king, move.item, 0, leafMode);
			move = getNextItem(move);
		}
	}

}


void initBoard(board* const b) {
	
	quadboard* const qb = &(b->quad);
	
	clearBoard(b);
	
	addPieces(qb, 255ULL << (8 * 1), PAWN | WHITE);
	addPieces(qb, 255ULL << (8 * 6), PAWN | BLACK);

	addPieces(qb, (128ULL + 1), ROOK | WHITE);
	addPieces(qb, (128ULL + 1) << (8 * 7), ROOK | BLACK);

	addPieces(qb, (64ULL + 2), KNIGHT | WHITE);
	addPieces(qb, (64ULL + 2) << (8 * 7), KNIGHT | BLACK);

	addPieces(qb, (32ULL + 4), BISHOP | WHITE);
	addPieces(qb, (32ULL + 4) << (8 * 7), BISHOP | BLACK);

	addPieces(qb, 16ULL, QUEEN | WHITE);
	addPieces(qb, 16ULL << (8 * 7), QUEEN | BLACK);

	addPieces(qb, 8ULL, KING | WHITE);
	addPieces(qb, 8ULL << (8 * 7), KING | BLACK);

	b->piecesMoved = 0;
	b->whosTurn = WHITE;
}

