#define WHITE ((byte)0)
#define BLACK ((byte)1)

#define WHITE_QUEENSIDE_CASTLE_MOVED	128
#define WHITE_KING_MOVED				64
#define WHITE_KINGSIDE_CASTLE_MOVED		32
#define BLACK_QUEENSIDE_CASTLE_MOVED	16
#define BLACK_KING_MOVED				8
#define BLACK_KINGSIDE_CASTLE_MOVED		4

#define BOARD_LEGAL (1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL (0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)

//#define boardAt(b,x,y) (b)->squares[(x)][(y)]
//#define boardAtSq(b,sq) (b)->squares[(sq).x][(sq).y]

typedef struct {
	quadboard quad;
	bitboard enemyCheckingmap; // Used to check castling rights.
	byte piecesMoved;  		// KINGS and CASTLES tracked here.
	byte whosTurn;     		// 0 = WHITE, 1 = BLACK.
} board;

void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
}

//
// Returns a map of all squares in check.
//
bitboard generateCheckingMap(quadboard qb, int team) {

	bitboard softBlockers = getFrenemies(qb);
	
	return
		// SLIDING PIECES
		  multiPieceAttacks(getQueens(qb, team),  softBlockers, 0ULL, nw | n | ne | w, ATTACKMODE_SLIDING)
		| multiPieceAttacks(getRooks(qb, team),   softBlockers, 0ULL,      n      | w, ATTACKMODE_SLIDING)
		| multiPieceAttacks(getBishops(qb, team), softBlockers, 0ULL, nw     | ne    , ATTACKMODE_SLIDING)
		
		// SINGLE AND PAWN PIECES
		| multiPieceAttacks(getKings(qb, team),   softBlockers, 0ULL, nw | n | ne | w, ATTACKMODE_SINGLE)
		| multiPieceAttacks(getPawns(qb, team),   softBlockers, 0ULL, nw     | ne    , ATTACKMODE_PAWN)
 		| multiPieceAttacks(getKnights(qb, team), softBlockers, 0ULL, nww | nnw |nne | nee, ATTACKMODE_SINGLE);
}

int isBoardLegal(board* b, int team) {
	return & generateCheckingMap(b->quad, 1 - team);
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
}

int spawnBoard(const board* const old, board* const new, const bitboard from, const bitboard to, const int promoteTo) {

	// To create a new board from a current board, first copy the current board's content to the new one.
	memcpy((void*)new, (void*)old, sizeof(board));

	// Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
	moveSquare(new->quad, from, to);
	
	//
	// To see if it's still possible to castle, we track whether the relevant pieces have moved.
	//
	switch(from) {
		case 0: 			new->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
		case 4: 			new->piecesMoved |= WHITE_KING_MOVED; break;
		case 7: 			new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED; break;
		case 7 * 256 + 0: 	new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
		case 7 * 256 + 4: 	new->piecesMoved |= BLACK_KING_MOVED; break;
		case 7 * 256 + 7: 	new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED; break;
	}

	//
	// Pawn promotion logic pt2 (takes the chosen promotion and applies it)
	//
	if (promoteTo > 0) {
		resetSquares(new->quad, to);
		switch(promoteTo) {
			case QUEEN:  addQueens(new->quad, to);	break;
			case ROOK:   addRooks(new->quad, to);	break;
			case BISHOP: addBishops(new->quad, to);	break;
			case KNIGHT: addKnights(new->quad, to);	break;
		}
	}

	//
	// Castling Logic pt2 (moves the castle over the king).
	//
	if (getKings(b->whosTurn) & from) {
		if ((from % 8) - (to % 8) == 2) {
			// Kingside castle detected
			moveSquare(new->quad, 1ULL, 4ULL);
		}
		else if ((from % 8) - (to % 8) == -2) {
			// Queenside castle detected
			moveSquare(new->quad, 128ULL, 16ULL);
		}
	}

	//
	// ============================================ CHANGE OF TURN ==================================================
	//
	b->whosTurn = 1 - whosTurn;
	//
	//
	//

	// We need this for checking to see if
	//
	// a) the move is even legal
	// b) whether, in the next move, a castling move is allowed
	//
	new->enemyCheckingmap = generateCheckingMap(new->quad, 1 - b->whosTurn);

	// Now we can perform the legality check
	if (getKings(b->quad, b->whosTurn) & new->enemyCheckingmap) {
		return BOARD_LEGAL;
	}
	else {
		return BOARD_NOT_LEGAL;
	}
	
}



int isSquareRangeChecked(const board* const b, const int xleft, const int xright, const int y, const byte team) {

	moveList attacks;

	// Get a list of opponent moves for nextBoard.
	buildMoveList(&attacks, b, opponentOf(team), MODE_ATTACK_LIST);
	
	int j,x;
	for (j = 0; j < attacks.ix; j++) {
		for (x = xleft; x <= xright; x++) {
			if (attacks.moves[j].to.x == x && attacks.moves[j].to.y == y) {
				return 1;
			}
		}
	}
	return 0;
	
}

int isCastlingMove(const board* const new, const move mv) {
	
	if (typeOf(boardAtSq(new,mv.to)) == KING && mv.from.x == 4) {  
	
		logg("Detected potential castling move\n");
	
		// Kingside Castle cleanup - i.e. move the kingside castle to it's new spot.
		if (mv.to.x == 6) {
			return KINGSIDE_CASTLE_MOVE;
		}
		
		// Queenside Castle cleanup - i.e. move the queenside castle to it's new spot.
		if (mv.to.x == 2) {
			return QUEENSIDE_CASTLE_MOVE;
		}
	}
	return NOT_A_CASTLE_MOVE;
}
