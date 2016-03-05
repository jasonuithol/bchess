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

#define PAWN 	((byte)1)	// 001
#define ROOK 	((byte)2)	// 010
#define KNIGHT 	((byte)3)	// 011
#define BISHOP 	((byte)4)	// 100
#define QUEEN 	((byte)5)	// 101
#define KING 	((byte)6)	// 110

typedef struct {
	bitboard team;
	bitboard type0;
	bitboard type1;
	bitboard type2;
} quadboard;


//
// For generic getXXXX methods, where XXX is a pieceType.
//
typedef bitboard (getterFuncPtr)(const quadboard qb, const byte team);


void printQB(const quadboard qb) {
	
	for (byte i = 0; i < 64; i++) {
		
		byte team,type0,type1,type2;
		
		// Mask, then shift down to equal 1 (or 0).
		team = (qb.team & (1ULL << i)) >> i;
		type0 = (qb.type0 & (1ULL << i)) >> i;
		type1 = (qb.type1 & (1ULL << i)) >> i;
		type2 = (qb.type2 & (1ULL << i)) >> i;

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
// pieceType is a horizontal bitfield corresponding to the
// vertical layout of the quadboard bits.
//
// bit 3: team		8		
// bit 2: type0		4
// bit 1: type1		2
// bit 0: type2		1
//
bitboard getPieces(const quadboard qb, const byte pieceType) {
	return (pieceType & 8 ? qb.team  : ~qb.team)
		 & (pieceType & 4 ? qb.type2 : ~qb.type2) 
		 & (pieceType & 2 ? qb.type1 : ~qb.type1) 
		 & (pieceType & 1 ? qb.type0 : ~qb.type0); 
}

//
// PRECONDITION: Target squares MUST BE KNOWN TO BE 0000 !!!!
//
void addPieces(quadboard* const qb, const bitboard pieces, const byte pieceType) {
	qb->team  |= pieceType & 8 ? pieces : 0;	
	qb->type2 |= pieceType & 4 ? pieces : 0;	
	qb->type1 |= pieceType & 2 ? pieces : 0;	
	qb->type0 |= pieceType & 1 ? pieces : 0;	
}

bitboard getPawns(const quadboard qb, const byte team) { // 001
	return  (team ? qb.team : ~qb.team) // 0 = white, 1 = black
			& ~qb.type0 
			& ~qb.type1 
			&  qb.type2;
}  

void addPawns(quadboard* const qb, const bitboard pieces, const byte team) {
	qb->team |= (team ? pieces : 0); // 0 = white, 1 = black
	qb->type0 &= ~pieces; // 0
	qb->type1 &= ~pieces; // 0
	qb->type2 |= pieces;  // 1
}

bitboard getRooks(const quadboard qb, const byte team) { // 010
	return  (team ? qb.team : ~qb.team)
			& ~qb.type0 
			&  qb.type1 
			& ~qb.type2;
}  

void addRooks(quadboard* const qb, const bitboard pieces, const byte team) {
	qb->team |= (team ? pieces : 0);
	qb->type0 &= ~pieces; // 0
	qb->type1 |= pieces;  // 1
	qb->type2 &= ~pieces; // 0
}

bitboard getKnights(const quadboard qb, const byte team) { // 011
	return  (team ? qb.team : ~qb.team)
			& ~qb.type0 
			&  qb.type1 
			&  qb.type2;
}  

void addKnights(quadboard* const qb, const bitboard pieces, const byte team) {
	qb->team |= (team ? pieces : 0);
	qb->type0 &= ~pieces; // 0
	qb->type1 |= pieces;  // 1
	qb->type2 |= pieces;  // 1
}

bitboard getBishops(const quadboard qb, const byte team) { // 100
	return  (team ? qb.team : ~qb.team)
			&  qb.type0 
			& ~qb.type1 
			& ~qb.type2;
}  

void addBishops(quadboard* const qb, const bitboard pieces, const byte team) {
	qb->team |= (team ? pieces : 0);
	qb->type0 |= pieces;  // 1
	qb->type1 &= ~pieces; // 0
	qb->type2 &= ~pieces; // 0
}

bitboard getQueens(const quadboard qb, const byte team) { // 101
	return  (team ? qb.team : ~qb.team)
			&  qb.type0 
			& ~qb.type1 
			&  qb.type2;
}  

void addQueens(quadboard* const qb, const bitboard pieces, const byte team) {
	qb->team |= (team ? pieces : 0);
	qb->type0 |= pieces;  // 1
	qb->type1 &= ~pieces; // 0
	qb->type2 |= pieces;  // 1
}

bitboard getKings(const quadboard qb, const byte team) { // 110
	return  (team ? qb.team : ~qb.team)
			&  qb.type0 
			&  qb.type1 
			& ~qb.type2;
}  

void addKings(quadboard* const qb, const bitboard pieces, const byte team) {
	qb->team |= (team ? pieces : 0);
	qb->type0 |= pieces;  // 1
	qb->type1 |= pieces;  // 1
	qb->type2 &= ~pieces; // 0
}

void resetSquares(quadboard* const qb, const bitboard squares) {
	qb->team  &= ~squares;
	qb->type0 &= ~squares;
	qb->type1 &= ~squares;
	qb->type2 &= ~squares;
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


bitboard getFriends(const quadboard qb, const byte team) {
	return (team ? ~team : team) & (qb.type0 | qb.type1 | qb.type2);
}

bitboard getEnemies(const quadboard qb, const byte team) {
	return (team ? team : ~team) & (qb.type0 | qb.type1 | qb.type2);
}

bitboard getFrenemies(const quadboard qb) {
	return qb.type0 | qb.type1 | qb.type2;
}

