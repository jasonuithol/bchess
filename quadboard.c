// ==================================================================
//
// quadboard.c
//
// Author: 		Jason Uithol
// Copyright: 	2016
//
//
// ==================================================================


#define WHITE ((byte)0)
#define BLACK ((byte)1)

//
// The rightmost (LSB) bit in these piece values is reserved for the team value.
//
#define PAWN 	((byte)2)	// 0010
#define ROOK 	((byte)4)	// 0100
#define KNIGHT 	((byte)6)	// 0110
#define BISHOP 	((byte)8)	// 1000
#define QUEEN 	((byte)10)	// 1010
#define KING 	((byte)12)	// 1100

byte opponentOf(byte team) {
	return team ^ 1;
}

typedef struct {
	bitboard type0;
	bitboard type1;
	bitboard type2;
	bitboard team;
} quadboard;

byte areEqualQB(quadboard a, quadboard b) {
	return a.team == b.team
		&& a.type0 == b.type0
		&& a.type1 == b.type1
		&& a.type2 == b.type2;	
}

void printByte(const byte v) {
	for (byte i = 0; i < 8; i++) {
		printf(v & 128U >> i ? "1" : "0");
	} 
	printf("\n");
}

byte getTeam(const quadboard qb, offset i) {
	return (qb.team & (1ULL << i)) >> i;
}

byte getType(const quadboard qb, offset i) {
	
	byte type0,type1,type2;
	
	// Mask, then shift down to equal 1 (or 0).
	type0 = (qb.type0 & (1ULL << i)) >> i;
	type1 = (qb.type1 & (1ULL << i)) >> i;
	type2 = (qb.type2 & (1ULL << i)) >> i;

	// We are int's because debugging.
	return (type0 << 3) | (type1 << 2) | (type2 << 1);

}

void printQB(const quadboard qb) {
	
	for (byte j = 64; j > 0; j--) {

		byte i = j - 1;
		char c = '?';

		switch(getType(qb,i)) {
			case PAWN:   c = 'P'; break;	// 0010
			case ROOK:   c = 'R'; break;	// 0100
			case KNIGHT: c = 'N'; break;	// 0110
			case BISHOP: c = 'B'; break;	// 1000
			case QUEEN:  c = 'Q'; break;	// 1010
			case KING:   c = 'K'; break;	// 1100
			default:     c = ' '; break;	// 0000, (and in theory, >= 111x)
		}

		if (getTeam(qb,i) && c != '?') {
			// If BLACK, then decapitalise in order to tell the difference
			// between the two teams.
			c += 'a' - 'A';
		}
		
		printf("%c", c);

		if (i % 8 == 7) { 
			// We just printed at the end of the line, so CR LF plox.
			printf("\n");
		}
		
	}
}

#define UNICODESET_SOLID (0)
#define UNICODESET_GHOST (1)

void printTeamColor(const byte team) {
	if (team == WHITE) {
		print("\033[31m\033[1m");  // bright red
	} 
	else {
		print("\033[34m\033[1m");  // bright blue
	}
}

void printResetColors() {
	print("\033[49m\033[39m\033(B\033[m"); // TODO: Use terminfo/tput, no hardcoding plox
}

//void printDot() {
//	print("\u00B7");
//}

void printPieceUnicode(const byte type, const byte team, const int setToUse) {
		
	printTeamColor(team);
	
	if(setToUse == UNICODESET_GHOST) {
		switch (type) {
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
		switch (type) {
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


void printQBUnicode(const quadboard qb) {
	
	byte color = 0;
	
	for (byte j = 64; j > 0; j--) {

		byte i = j - 1;

		// Set the color of the square
		if (color == 0) {
			// Reverse
			print("\033[47m"); // TODO: Use terminfo/tput, no hardcoding plox
		}
		else {
			// Normal
			print("\033[40m"); // TODO: Use terminfo/tput, no hardcoding plox
		}

		print(" ");
		printPieceUnicode(getType(qb,i), getTeam(qb,i), UNICODESET_SOLID);
		print(" ");

		if (i % 8 == 0) { 
			// Reset the color codes, and start a new line.
			printResetColors();
 			print("\n");
		}
		else {
			color = 1 - color;
		}
		
	}
}





//
// PRECONDITION: Target squares MUST BE KNOWN TO BE 0000 !!!!
//
void addPieces(quadboard* const qb, const bitboard pieces, const byte pieceType) {
	qb->type0 |= (pieceType & 8 ? pieces : 0);	
	qb->type1 |= (pieceType & 4 ? pieces : 0);	
	qb->type2 |= (pieceType & 2 ? pieces : 0);	
	qb->team  |= (pieceType & 1 ? pieces : 0);	
}

bitboard getPieces(const quadboard qb, const byte pieceType) {
	return (pieceType & 8 ? qb.type0 : ~qb.type0) 
		 & (pieceType & 4 ? qb.type1 : ~qb.type1) 
		 & (pieceType & 2 ? qb.type2 : ~qb.type2) 
		 & (pieceType & 1 ? qb.team  : ~qb.team);
}

void resetSquares(quadboard* const qb, const bitboard squares) {
	qb->type0 &= ~squares;
	qb->type1 &= ~squares;
	qb->type2 &= ~squares;
	qb->team  &= ~squares;
}

void moveSquare(quadboard* const qb, const bitboard from, const bitboard to) {

	// Write 0000 to target square.
	resetSquares(qb, to);
	
	// Set appropriate 1's to target square.
	qb->type0 |= (qb->type0 & from) ? to : 0;
	qb->type1 |= (qb->type1 & from) ? to : 0;
	qb->type2 |= (qb->type2 & from) ? to : 0;
	qb->team  |= (qb->team  & from) ? to : 0;
	
	// Write 0000 to source square.
	resetSquares(qb, from);
}

bitboard getAllPieces(const quadboard qb) {
	return qb.type0 | qb.type1 | qb.type2;
}

bitboard getTeamPieces(const quadboard qb, const byte team) {
	return (team ? qb.team : ~qb.team) & getAllPieces(qb);
}
