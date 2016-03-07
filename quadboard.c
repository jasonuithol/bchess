// ==================================================================
//
// quadboard.c
//
// Author: 		Jason Uithol
// Copyright: 	2016
//
// State Use: 
//
//	4: pieceType - padded to 1 byte
//
//			  00000000 00000000 00000000 00000000
//			  00000000 00000000 00000000 0000TTTT
//
// ==================================================================


#define WHITE ((byte)0)
#define BLACK ((byte)1)

#define PAWN 	((byte)1)	// 0010
#define ROOK 	((byte)2)	// 0100
#define KNIGHT 	((byte)3)	// 0110
#define BISHOP 	((byte)4)	// 1000
#define QUEEN 	((byte)5)	// 1010
#define KING 	((byte)6)	// 1100

byte opponentOf(byte team) {
	return team ^ 1;
}

typedef struct {
	bitboard type0;
	bitboard type1;
	bitboard type2;
	bitboard team;
} quadboard;


void printQB(const quadboard qb) {
	
	// Flip the board so that white is at the bottom.
	bitboard type0BB  = flipBoardVert(qb.type0);
	bitboard type1BB  = flipBoardVert(qb.type1);
	bitboard type2BB  = flipBoardVert(qb.type2);
	bitboard teamBB   = flipBoardVert(qb.team);
	
	for (byte i = 0; i < 64; i++) {
		
		byte type0,type1,type2,team;
		
		// Mask, then shift down to equal 1 (or 0).
		type0 = (type0BB & (1ULL << i)) >> i;
		type1 = (type1BB & (1ULL << i)) >> i;
		type2 = (type2BB & (1ULL << i)) >> i;
		team  = (teamBB  & (1ULL << i)) >> i;

		// We are int's because debugging.
		int type = (type0 << 2) | (type1 << 1) | type2;
		
		char c = '?';

		switch(type) {
			case PAWN:   c = 'P'; break;	// 001
			case ROOK:   c = 'R'; break;	// 010
			case KNIGHT: c = 'N'; break;	// 011
			case BISHOP: c = 'B'; break;	// 100
			case QUEEN:  c = 'Q'; break;	// 101
			case KING:   c = 'K'; break;	// 110
			default:     c = ' '; break;	// 000, (and in theory, >= 111)
		}

		if (team && c != '?') {
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


//
// PRECONDITION: Target squares MUST BE KNOWN TO BE 0000 !!!!
//
void addPieces(quadboard* const qb, const bitboard pieces, const byte pieceType) {
	qb->type2 |= pieceType & 8 ? pieces : 0;	
	qb->type1 |= pieceType & 4 ? pieces : 0;	
	qb->type0 |= pieceType & 2 ? pieces : 0;	
	qb->team  |= pieceType & 1 ? pieces : 0;	
}


//
// pieceType is a horizontal bitfield corresponding to the
// vertical layout of the quadboard bits.
//
// bit 3: team		8		
// bit 2: type0		4
// bit 1: type1		2
// bit 0: type2		1
//
bitboard getPieces(const quadboard qb, const byte pieceType) {
	return (pieceType & 8 ? qb.type2 : ~qb.type2) 
		 & (pieceType & 4 ? qb.type1 : ~qb.type1) 
		 & (pieceType & 2 ? qb.type0 : ~qb.type0) 
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
	qb->team  |= (qb->team  & from) ? to : 0;
	qb->type0 |= (qb->type0 & from) ? to : 0;
	qb->type1 |= (qb->type1 & from) ? to : 0;
	qb->type2 |= (qb->type2 & from) ? to : 0;
	
	// Write 0000 to source square.
	resetSquares(qb, from);
}

bitboard getAllPieces(const quadboard qb) {
	return qb.type0 | qb.type1 | qb.type2;
}

bitboard getTeamPieces(const quadboard qb, const byte team) {
	return (team ? qb.team : ~qb.team) & getAllPieces(qb);
}
