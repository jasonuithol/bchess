// ================================================================
//
// board.c
//
// The storage and printing of board state is done here.
// Move history located here too.
//

typedef struct {
	byte x;
	byte y;
} square;

#define copySquare(copy,orig) (copy).x = (orig).x;(copy).y = (orig).y

#define WHITE_QUEENSIDE_CASTLE_MOVED	128
#define WHITE_KING_MOVED				64
#define WHITE_KINGSIDE_CASTLE_MOVED		32
#define BLACK_QUEENSIDE_CASTLE_MOVED	16
#define BLACK_KING_MOVED				8
#define BLACK_KINGSIDE_CASTLE_MOVED		4

typedef struct {
	byte squares[8][8];
	byte whosTurn;
	byte piecesMoved;
} board;

#define boardAt(b,x,y) (b)->squares[(x)][(y)]
#define boardAtSq(b,sq) (b)->squares[(sq).x][(sq).y]

void clearBoard(board* b) {
	int x,y;
	for (x = 0; x < 8; x++) {
		for (y = 0; y < 8; y++) {
			b->squares[x][y] = 0;
		}
	}
	b->piecesMoved = 0;
	b->whosTurn = WHITE;
}

void crashTest(board* b) {
	
	clearBoard(b);
	
	boardAt(b,1,1) = WHITE + KING;
	
	boardAt(b,1,4) = BLACK + KING;
	boardAt(b,2,3) = BLACK + PAWN;
	boardAt(b,3,5) = BLACK + PAWN;
	boardAt(b,4,2) = BLACK + QUEEN;
	boardAt(b,5,4) = BLACK + BISHOP;
	
	b->whosTurn = BLACK;
	b->piecesMoved = BLACK_KING_MOVED + WHITE_KING_MOVED;
}

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

void initBoard(board* b) {

	b->squares[0][0] = WHITE + ROOK;
	b->squares[1][0] = WHITE + KNIGHT;
	b->squares[2][0] = WHITE + BISHOP;
	b->squares[3][0] = WHITE + QUEEN;
	b->squares[4][0] = WHITE + KING;
	b->squares[5][0] = WHITE + BISHOP;
	b->squares[6][0] = WHITE + KNIGHT;
	b->squares[7][0] = WHITE + ROOK;

	b->squares[0][7] = BLACK + ROOK;
	b->squares[1][7] = BLACK + KNIGHT;
	b->squares[2][7] = BLACK + BISHOP;
	b->squares[3][7] = BLACK + QUEEN;
	b->squares[4][7] = BLACK + KING;
	b->squares[5][7] = BLACK + BISHOP;
	b->squares[6][7] = BLACK + KNIGHT;
	b->squares[7][7] = BLACK + ROOK;

	int n;
	for (n=0;n<8;n++) {
		b->squares[n][1] = WHITE + PAWN;
		b->squares[n][2] = 0;
		b->squares[n][3] = 0;
		b->squares[n][4] = 0;
		b->squares[n][5] = 0;
		b->squares[n][6] = BLACK + PAWN;
	}

	b->whosTurn = WHITE;
	b->piecesMoved = 0;

}

void spawnBoard(board* old, board* new, square from, square to) {

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

void printSquare(square s) {
	print("[%d,%d]",s.x,s.y);
}

void printBoardToLog(board* b) {
	int x,y;
	byte p;
	for(y=7;y>=0;y--) {
		logg("-----------------------------------\n");
		for (x=0;x<8;x++) {
			p = b->squares[x][y];
			logg("| ");
			printPieceToLog(p);
			logg(" ");
		}
		logg("|\n");
	}
	logg("-----------------------------------\n\n");
	logg("Historical move flags: %d\n",b->piecesMoved);
	logg("Who's turn ? %d\n", b->whosTurn);
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
	print(" to move.\n");
}



