// ================================================================
//
// board.c
//
// The storage and printing of board state is done here.
// Move history located here too.
//

#define WHITE ((byte)0)
#define BLACK ((byte)1)

#define WHITE_QUEENSIDE_CASTLE_MOVED	128
#define WHITE_KING_MOVED				64
#define WHITE_KINGSIDE_CASTLE_MOVED		32
#define BLACK_QUEENSIDE_CASTLE_MOVED	16
#define BLACK_KING_MOVED				8
#define BLACK_KINGSIDE_CASTLE_MOVED		4

//#define boardAt(b,x,y) (b)->squares[(x)][(y)]
//#define boardAtSq(b,sq) (b)->squares[(sq).x][(sq).y]

typedef struct {
	quadboard quad;
	byte piecesMoved;  // KINGS and CASTLES tracked here.
	byte whosTurn;     // 0 = WHITE, 1 = BLACK.
} board;

void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
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

/*
void pawnPromotionTest(board* b) {
	//
	// Pawn Promotion Test
	//
	int n;
	for (n=0;n<8;n++) {
		b->squares[n][0] = 0;
		b->squares[n][1] = 0;
		b->squares[n][2] = 0;
		b->squares[n][3] = 0;
		b->squares[n][4] = 0;
		b->squares[n][5] = 0;
		b->squares[n][6] = 0;
		b->squares[n][7] = 0;
	}

	b->squares[0][5] = WHITE + PAWN;
	b->squares[1][5] = WHITE + PAWN;
	b->squares[4][0] = WHITE + KING;
	b->squares[7][7] = BLACK + KING;
//	b->squares[7][6] = BLACK + PAWN;

	b->whosTurn = WHITE;
	b->piecesMoved = BLACK_KING_MOVED + WHITE_KING_MOVED;
}

void profilingBoard(board *b) {

	clearBoard(b);

	b->squares[0][0] = WHITE + ROOK;
	b->squares[1][0] = WHITE + KNIGHT;
	b->squares[2][0] = WHITE + BISHOP;
	b->squares[3][0] = WHITE + QUEEN;
	b->squares[4][0] = WHITE + KING;
	b->squares[2][3] = WHITE + BISHOP; // moved to c4
	b->squares[5][2] = WHITE + KNIGHT; // moved to f3
	b->squares[7][0] = WHITE + ROOK;

	b->squares[0][7] = BLACK + ROOK;
	b->squares[2][5] = BLACK + KNIGHT; // moved to c6
	b->squares[2][7] = BLACK + BISHOP;
	b->squares[5][5] = BLACK + QUEEN;  // moved to f6
	b->squares[4][7] = BLACK + KING;
	b->squares[1][3] = BLACK + BISHOP; // moved to b4
	b->squares[6][7] = BLACK + KNIGHT;
	b->squares[7][7] = BLACK + ROOK;

	int n;
	for (n=0;n<8;n++) {
		b->squares[n][1] = WHITE + PAWN;
		b->squares[n][6] = BLACK + PAWN;
	}

	b->squares[1][2] = WHITE + PAWN; b->squares[1][1] = 0; // b3
	b->squares[4][3] = WHITE + PAWN; b->squares[4][1] = 0; // e4
	b->squares[4][4] = BLACK + PAWN; b->squares[4][6] = 0; // e5

	b->whosTurn = WHITE;
	b->piecesMoved = 0;
}

void spawnBoard(const board* const old, board* const new, const square from, const square to) {

	// To create a new board from a current board, first copy the current board's content to the new one.
	memcpy((void*)new, (void*)old, sizeof(board));

	// Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
	byte mover = new->squares[from.x][from.y];
	new->squares[to.x][to.y] = mover;
	new->squares[from.x][from.y] = 0;
	new->whosTurn = opponentOf(teamOf(mover));

	//
	// To see if it's still possible to castle, we track whether the relevant pieces have moved.
	//
	switch(from.y * 256 + from.x) {
		case 0: 			new->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
		case 4: 			new->piecesMoved |= WHITE_KING_MOVED; break;
		case 7: 			new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED; break;
		case 7 * 256 + 0: 	new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
		case 7 * 256 + 4: 	new->piecesMoved |= BLACK_KING_MOVED; break;
		case 7 * 256 + 7: 	new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED; break;
	}

}

void printBoardUnicode(board* b) {
	int x,y;
	byte p;
	print("\n");
	for(y=7;y>=0;y--) {
		for (x=0;x<8;x++) {
			
			// Set the color of the square
			if ((x + y) % 2 == 1) {
				// Reverse
				print("\033[47m"); // TODO: Use terminfo/tput, no hardcoding plox
			}
			else {
				// Normal
				print("\033[40m"); // TODO: Use terminfo/tput, no hardcoding plox
			}
			print(" ");
			p = b->squares[x][y];
			printPieceUnicode(p, UNICODESET_SOLID);
			print(" ");
		}
		// Reset the color codes.
		print("\033[49m\033[39m\033(B\033[m"); // TODO: Use terminfo/tput, no hardcoding plox
		print("\n");
	}
	
	print("\n");
	printTeam(b->whosTurn);
	print(" to move.\n");
	
}

*/

