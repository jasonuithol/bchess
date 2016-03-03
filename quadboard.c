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

void printQB(const quadboard qb) {
	
	for (int i = 0; i < 64; i++) {
		
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

bitboard getPawns(quadboard qb, int team) { // 001
	return  (team ? ~qb.team : qb.team) // 0 = white, 1 = black
			& ~qb.type0 
			& ~qb.type1 
			&  qb.type2;
}  

void addPawns(quadboard* qb, bitboard pieces, int team) {
	qb->team |= (team ? 0 : pieces); // 0 = white, 1 = black
	qb->type0 &= ~pieces; // 0
	qb->type1 &= ~pieces; // 0
	qb->type2 |= pieces;  // 1
}

bitboard getRooks(quadboard qb, int team) { // 010
	return  (team ? ~qb.team : qb.team)
			& ~qb.type0 
			&  qb.type1 
			& ~qb.type2;
}  

void addRooks(quadboard* qb, bitboard pieces, int team) {
	qb->team |= (team ? 0 : pieces);
	qb->type0 &= ~pieces; // 0
	qb->type1 |= pieces;  // 1
	qb->type2 &= ~pieces; // 0
}

bitboard getKnights(quadboard qb, int team) { // 011
	return  (team ? ~qb.team : qb.team)
			& ~qb.type0 
			&  qb.type1 
			&  qb.type2;
}  

void addKnights(quadboard* qb, bitboard pieces, int team) {
	qb->team |= (team ? 0 : pieces);
	qb->type0 &= ~pieces; // 0
	qb->type1 |= pieces;  // 1
	qb->type2 |= pieces;  // 1
}

bitboard getBishops(quadboard qb, int team) { // 100
	return  (team ? ~qb.team : qb.team)
			&  qb.type0 
			& ~qb.type1 
			& ~qb.type2;
}  

void addBishops(quadboard* qb, bitboard pieces, int team) {
	qb->team |= (team ? 0 : pieces);
	qb->type0 |= pieces;  // 1
	qb->type1 &= ~pieces; // 0
	qb->type2 &= ~pieces; // 0
}

bitboard getQueens(quadboard qb, int team) { // 101
	return  (team ? ~qb.team : qb.team)
			&  qb.type0 
			& ~qb.type1 
			&  qb.type2;
}  

void addQueens(quadboard* qb, bitboard pieces, int team) {
	qb->team |= (team ? 0 : pieces);
	qb->type0 |= pieces;  // 1
	qb->type1 &= ~pieces; // 0
	qb->type2 |= pieces;  // 1
}

bitboard getKings(quadboard qb, int team) { // 110
	return  (team ? ~qb.team : qb.team)
			&  qb.type0 
			&  qb.type1 
			& ~qb.type2;
}  

void addKings(quadboard* qb, bitboard pieces, int team) {
	qb->team |= (team ? 0 : pieces);
	qb->type0 |= pieces;  // 1
	qb->type1 |= pieces;  // 1
	qb->type2 &= ~pieces; // 0
}

void resetSquares(quadboard* qb, bitboard squares) {
	qb->team &= ~squares;
	qb->type0 &= ~squares;
	qb->type1 &= ~squares;
	qb->type2 &= ~squares;
}

void moveSquare(quadboard* qb, bitboard from, bitboard to) {

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


bitboard getFriends(quadboard qb, int team) {
	return (team ? ~team : team) & (qb.type0 | qb.type1 | qb.type2);
}

bitboard getEnemies(quadboard qb, int team) {
	return (team ? team : ~team) & (qb.type0 | qb.type1 | qb.type2);
}

bitboard getFrenemies(quadboard qb) {
	return qb.type0 | qb.type1 | qb.type2;
}

