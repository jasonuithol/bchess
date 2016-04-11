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

//typedef __m128i doubleboard;


typedef struct {
	bitboard type0; // bit 3 in pieceType values
	bitboard type1; // bit 2 in pieceType values
	bitboard type2; // bit 1 in pieceType values
	bitboard team;  // bit 0 in pieceType values
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
// Returns 11111111 11111111 11111111 11111111 11111111 11111111 11111111 11111111 
// if pieceType has bitPosition bit set to 1
// else returns 0
//
bitboard get64Mask(const byte pieceType, const offset bitPosition) {
	// Start with all 64 11111's
	// Shift bitPosition bit to the least significant bit 
	//  (after masking out all the other bits)
	// Shift it back to 6th position (is now 0 or 64)
	// Now shift the 11111's value either 0 or 64 times
	// Now invert, because we wanted the opposite
	
//	print("Received pieceType %u and bitPosition %u\n", pieceType, bitPosition);

	const byte typeMask = (1 << bitPosition);
	const byte isSet 	= (pieceType & typeMask) >> bitPosition;
//	const offset off 	= (isSet ^ 1) << 6;
	const bitboard mask64 = ~0ULL;

//	print("Calculated typeMask %u, isSet %u, off %u\n", typeMask, isSet, off);

//	print("Mask before shifting\n");
//	printBB(mask64);
//	print("\n");

	const bitboard result = mask64 * isSet;

//	print("Mask after shifting\n");
//	printBB(result);
//	print("\n");

	return result;
	
//	return ~((~0ULL) << (((pieceType & (1 << bitPosition)) >> bitPosition) << 6));
}


void andBBMask(quadboard* const qb, const bitboard bb) {

	qb->type2 &= bb; 	
	qb->type1 &= bb; 
	qb->type0 &= bb; 
	qb->team  &= bb; 
}    

void orQBMask(quadboard* const qb, const quadboard* const mask) {

	qb->type2 |= mask->type2; 
	qb->type1 |= mask->type1; 
	qb->type0 |= mask->type0; 
	qb->team  |= mask->team; 
}    

void xorQBMask(quadboard* const qb, const quadboard* const mask) {

	qb->type2 ^= mask->type2; 
	qb->type1 ^= mask->type1; 
	qb->type0 ^= mask->type0; 
	qb->team  ^= mask->team; 
}    

void notQB(quadboard* const qb) {

	qb->type2 = ~qb->type2; 
	qb->type1 = ~qb->type1; 
	qb->type0 = ~qb->type0; 
	qb->team  = ~qb->team; 
}    

void setQuadMask(quadboard* const mask, const byte pieceType) {

	mask->type0 = get64Mask(pieceType, 3);
	mask->type1 = get64Mask(pieceType, 2);
	mask->type2 = get64Mask(pieceType, 1);
	mask->team  = get64Mask(pieceType, 0);
}
  
//
// PRECONDITION: Target squares MUST BE KNOWN TO BE 0000 !!!!
//
void addPieces(quadboard* const qb, const bitboard pieces, const byte pieceType) {

	quadboard mask;
	setQuadMask(&mask, pieceType);

	andBBMask(&mask, pieces);
	orQBMask(qb, &mask);
}

bitboard getPieces(quadboard qb, const byte pieceType) {
	
	quadboard mask;
	setQuadMask(&mask, pieceType);
	notQB(&mask);
	xorQBMask(&qb, &mask);
	return qb.type2 & qb.type1 & qb.type0 & qb.team;
/*	
	return (get64Mask(pieceType, 3) ^ qb.type0)
		 & (get64Mask(pieceType, 2) ^ qb.type1)
		 & (get64Mask(pieceType, 1) ^ qb.type2)
		 & (get64Mask(pieceType, 0) ^ qb.team)
	;
*/ 
}

void resetSquares(quadboard* const qb, const bitboard squares) {
	qb->type0 &= ~squares;
	qb->type1 &= ~squares;
	qb->type2 &= ~squares;
	qb->team  &= ~squares;
}

bitboard moveBitValue(const bitboard field, const bitboard from, const bitboard to) {
	return (populationCount(field & from) * ~0ULL) & to;
}

void moveSquare(quadboard* const qb, const bitboard from, const bitboard to) {

	// Write 0000 to target square.
	resetSquares(qb, to);

	byte pieceType = getType(*qb, trailingBit_Bitboard(from));
	addPieces(qb,to,pieceType);
	
	// Set appropriate 1's to target square.
//	qb->type0 = moveBitValue(qb->type0, from, to);
//	qb->type1 = moveBitValue(qb->type1, from, to);
//	qb->type2 = moveBitValue(qb->type2, from, to);
//	qb->team  = moveBitValue(qb->team,  from, to);
	
	// Write 0000 to source square.
	resetSquares(qb, from);
}

bitboard getAllPieces(const quadboard qb) {
	return qb.type0 | qb.type1 | qb.type2;
}

bitboard getTeamPieces(const quadboard qb, const byte team) {
	const bitboard teamMask = (~0ULL * team) ^ qb.team;
		
	return (~teamMask) & getAllPieces(qb);
}

#ifdef BUILD_TESTING

void test_quadboard() {
		
	print("Expecting all 1111's\n");
	printBB(get64Mask(8,3));
	printBB(get64Mask(4,2));
	printBB(get64Mask(2,1));
	printBB(get64Mask(1,0));
	
	print("Expecting all 1111's\n");
	printBB(get64Mask(15,3));
	printBB(get64Mask(15,2));
	printBB(get64Mask(15,1));
	printBB(get64Mask(15,0));

	print("Expecting all 0's\n");
	printBB(get64Mask(7,3));
	printBB(get64Mask(3,2));
	printBB(get64Mask(1,1));
	printBB(get64Mask(14,0));

	print("Expecting 128 0's, 64 1's, then 64 0's\n");
	printBB(get64Mask(2,3));
	printBB(get64Mask(2,2));
	printBB(get64Mask(2,1));
	printBB(get64Mask(2,0));
	
	if (get64Mask(0,0) != 0ULL) {
		error("get64Mask expected to return 0\n");
	}
	if (get64Mask(8ULL,3) != ~0ULL) {
		error("get64Mask expected to return ~0ULL\n");
	}
	
	
	
}

#endif



