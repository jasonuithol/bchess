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
			error("\nBad team value passed to opponentOf(): %d\n", (int)t);
			return -1;
	}
}

byte typeOf(byte p) {
	return p & ~(WHITE | BLACK);
}

void printTeam(byte t) {
	switch (t) {
		case WHITE:
			print("WHITE"); 
			break;
		case BLACK:
			print("BLACK"); 
			break;
		default:
			print("team? %d", (int)t); 
	}
}

void printType(byte t) {
	switch (t) {
		case KING :
			print("KING"); 
			break;
		case QUEEN : 
			print("QUEEN"); 
			break;
		case ROOK :
			print("ROOK"); 
			break;
		case BISHOP :
			print("BISHOP");
			break;
		case KNIGHT :
			print("KNIGHT");
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
	print(" ");
	printType(typeOf(p));
}

void printPieceToLog(byte p) {
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
	logg("%c",typeChar);
}

// This routine now means that gcc must be invoked thus
//
// gcc -std=c99 bchess.c
//
void printPieceUnicode(byte p) {
	
	const int SET_TO_USE = BLACK;
	
	if (teamOf(p) == WHITE) {
		print("\033[31m\033[1m");  // bright red
	} 
	else {
		print("\033[34m\033[1m");  // bright blue
	}
	
	if(SET_TO_USE == WHITE) {
		switch (typeOf(p)) {
			case 0: print(" ");break;
			case KING:   print("\u2654"); break;
			case QUEEN:  print("\u2655"); break;
			case BISHOP: print("\u2657"); break;
			case KNIGHT: print("\u2658"); break;
			case ROOK:   print("\u2656"); break;
			case PAWN:   print("\u2659"); break;
			default:     print("?"); 
		}
	}
	else {
		switch (typeOf(p)) {
			case 0: print(" ");break;
			case KING:   print("\u265A"); break;
			case QUEEN:  print("\u265B"); break;
			case BISHOP: print("\u265D"); break;
			case KNIGHT: print("\u265E"); break;
			case ROOK:   print("\u265C"); break;
			case PAWN:   print("\u265F"); break;
			default:     print("?"); 
		}
	}
}




