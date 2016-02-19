// ================================================================
//
// piece.c
//
// The type definitions and constant declarations for pieces are 
// here.  Also here are piece printing functions.
//


#define byte unsigned char

#define WHITE ((byte)128)
#define BLACK ((byte)64)

#define KING 	((byte)63)
#define QUEEN 	((byte)9)
#define ROOK 	((byte)7)
#define BISHOP 	((byte)5)
#define KNIGHT 	((byte)3)
#define PAWN 	((byte)1)


byte teamOf(byte p) {
	return p & (WHITE | BLACK);
}

byte opponentOf(byte t) {
	switch(t) {
		case 0:
			return 0;
		case WHITE:
			return BLACK;
		case BLACK:
			return WHITE;
		default:
			printf("\nBad team value passed to opponentOf(): %d\n", (int)t);
			return -1;
	}
}

byte typeOf(byte p) {
	return p & ~(WHITE | BLACK);
}

void printTeam(byte t) {
	switch (t) {
		case WHITE:
			printf("WHITE"); 
			break;
		case BLACK:
			printf("BLACK"); 
			break;
		default:
			printf("team? %d", (int)t); 
	}
}

void printType(byte t) {
	switch (t) {
		case KING :
			printf("KING"); 
			break;
		case QUEEN : 
			printf("QUEEN"); 
			break;
		case ROOK :
			printf("ROOK"); 
			break;
		case BISHOP :
			printf("BISHOP");
			break;
		case KNIGHT :
			printf("KNIGHT");
			break;
		case PAWN :
			printf("PAWN");
			break;
		default :
			printf("type? %d", (int)t);
	}
}

void printPiece(byte p) {
	printTeam(teamOf(p));
	printf(" ");
	printType(typeOf(p));
}

void printPieceClassic(byte p) {
	char teamCapitalOffset = 0;
	char typeChar = '?';
	if(teamOf(p) == BLACK) {
		teamCapitalOffset = 'a' - 'A';
	}
	switch (typeOf(p)) {
		case 0: typeChar = ' ';break;
		case KING: typeChar = 'K'; break;
		case QUEEN: typeChar = 'Q'; break;
		case BISHOP: typeChar = 'B'; break;
		case KNIGHT: typeChar = 'N'; break;
		case ROOK: typeChar = 'R'; break;
		case PAWN: typeChar = 'P'; break;
		default: typeChar = '?'; 
	}
	if (typeChar != '?' && typeChar != ' ') {
		typeChar += teamCapitalOffset;
	}
	printf("%c",typeChar);
}

// This routine now means that gcc must be invoked thus
//
// gcc -std=c99 bchess.c
//
void printPieceUnicode(byte p) {
	if(teamOf(p) == WHITE) {
		switch (typeOf(p)) {
			case 0: printf(" ");break;
			case KING:   printf("\u2654"); break;
			case QUEEN:  printf("\u2655"); break;
			case BISHOP: printf("\u2657"); break;
			case KNIGHT: printf("\u2658"); break;
			case ROOK:   printf("\u2656"); break;
			case PAWN:   printf("\u2659"); break;
			default:     printf("?"); 
		}
	}
	else {
		switch (typeOf(p)) {
			case 0: printf(" ");break;
			case KING:   printf("\u265A"); break;
			case QUEEN:  printf("\u265B"); break;
			case BISHOP: printf("\u265D"); break;
			case KNIGHT: printf("\u265E"); break;
			case ROOK:   printf("\u265C"); break;
			case PAWN:   printf("\u265F"); break;
			default:     printf("?"); 
		}
	}
}




