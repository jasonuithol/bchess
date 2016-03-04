
#define BOARD_LEGAL (1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL (0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)

typedef struct {
	quadboard quad;
	bitboard castlingCheckingMap; 	// Used to check castling ability.
	byte piecesMoved;  				// KINGS and CASTLES tracked here.
	byte whosTurn;     				// 0 = WHITE, 1 = BLACK.
} board;

void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
}


//
// All moves MUST be performed by this method to ensure that:
//
// * Castling is managed properly.
// * Pawns get promoted.
// * Illegal board positions (involving check) are thrown out.
//
byte spawnLeafBoard(const board* const old, board* const new, const bitboard from, const bitboard to, const byte promoteTo) {

	quadboard* qb = &(new->quad);

	// To create a new board from a current board, first copy the current board's content to the new one.
	memcpy((void*)new, (void*)old, sizeof(board));

	// Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
	moveSquare(qb, from, to);

	// Change the turn on the new board.  To get the team that just moved, use old->whosTurn.
	new->whosTurn = 1 - new->whosTurn;


	// Now we can perform the legality check
	// The checking map is useful exactly once, so we don't cache it.
	//
	if (!(getKings(new->quad, old->whosTurn) & generateCheckingMap(new->quad, new->whosTurn))) {
		return BOARD_NOT_LEGAL;
	}

	
	//
	// Pawn promotion followup (takes the chosen promotion and applies it)
	//
	if (promoteTo > 0) {
		resetSquares(qb, to);
		switch(promoteTo) {
			case QUEEN:  addQueens(qb, to, old->whosTurn);	break;
			case ROOK:   addRooks(qb, to, old->whosTurn);	break;
			case BISHOP: addBishops(qb, to, old->whosTurn);	break;
			case KNIGHT: addKnights(qb, to, old->whosTurn);	break;
		}
	}

	//
	// Castling followup (moves the castle over the king).
	//
	if (getKings(old->quad, old->whosTurn) & from) {
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
byte spawnFullBoard(const board* const old, board* const new, const bitboard from, const bitboard to, const byte promoteTo) {

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


	//
	// We need this for checking to see if in the next move, a castling move is allowed
	//
	// NOTE: FOR LEAF BOARDS, THIS IS A NEEDLESS COST !!!!!
	//
	new->castlingCheckingMap = generateCheckingMap(new->quad, old->whosTurn);


	// Board passed the illegal check state test earlier, so board is legal.
	return BOARD_LEGAL;

}

void initBoard(board* b) {
	
	quadboard* qb = &(b->quad);
	
	clearBoard(b);
	
	addPawns(qb, 255ULL << (8 * 1), WHITE);
	addPawns(qb, 255ULL << (8 * 6), BLACK);

	addRooks(qb, (128ULL + 1),            WHITE);
	addRooks(qb, (128ULL + 1) << (8 * 7), BLACK);

	addKnights(qb, (64ULL + 2), WHITE);
	addKnights(qb, (64ULL + 2) << (8 * 7), BLACK);

	addBishops(qb, (32ULL + 4), WHITE);
	addBishops(qb, (32ULL + 4) << (8 * 7), BLACK);

	addKings(qb, 16ULL, WHITE);
	addKings(qb, 16ULL << (8 * 7), BLACK);

	addQueens(qb, 8ULL, WHITE);
	addQueens(qb, 8ULL << (8 * 7), BLACK);

	b->piecesMoved = 0;
	b->whosTurn = WHITE;
	b->castlingCheckingMap = generateCheckingMap(b->quad, 1 - b->whosTurn);
}

