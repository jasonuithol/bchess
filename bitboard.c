#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

typedef uint8_t  byte;

//typedef uint8_t  bitrank;
//typedef uint8_t  coord;
typedef uint8_t  offset;
typedef int16_t  relativeOffset;
typedef uint64_t bitboard;

typedef struct {
	bitboard team;
	bitboard type0;
	bitboard type1;
	bitboard type2;
} quadboard;

/*
typedef union {
	bitboard value;
	bitrank rank[8];
} rankedBitboard;

typedef union {
	bitboard pieces[6];
	struct {
		bitboard pawns; 	// type 001
		bitboard rooks;		// type 010
		bitboard knights;   // type 011
		bitboard bishops;   // type 100
		bitboard queens;	// type 101
		bitboard king;		// type 110
	};
} teamboard;
*/

typedef struct {
	/*
	union { 
		struct {
			teamboard white;
			teamboard black;
		};
		teamboard team[2];
	};
	*/ 
	quadboard quad;
	byte piecesMoved;  // KINGS and CASTLES tracked here.
	byte whosTurn;     // 0 = WHITE, 1 = BLACK.
} board;

/*
typedef union {
	struct {
		coord x;
		coord y;
	};
	uint16_t value; 
} square;
*/


// Up and down go swapsies.
bitboard flipBoardVert(bitboard b) {
	return (bitboard)__builtin_bswap64((uint64_t)b);
}

/*
// I saw this on Stack Exchange, everyone ignored it.
// I think it's the best answer by miles
bitrank reverseRank(const bitrank b) {
	return (bitrank)(b * 0x0202020202ULL & 0x010884422010ULL) % 1023;	
}

// Left and right go swapsies.
bitboard flipBoardHoriz(const bitboard b) {
	
	rankedBitboard rb;
	rb.value = b;
	coord i;
	for (i = 0; i < 8; i++) {
		rb.rank[i] = reverseRank(rb.rank[i]);
	}
	return rb.value;
}

// Pull rankIx'th byte out of bitboard.  
bitrank getRankData(const bitboard b, const coord rankIx) {
	return ((rankedBitboard)b).rank[rankIx];	
}

// Shift a rank left (MSB wise) so that fileIx is new origin
bitrank setOrigin(const bitrank rankData, const coord fileIx) {
	return rankData << fileIx;
}

// Go from left to right searching for first position of a 1.
// This is a hardware instruction.
// A byte sized version of this instruction does not appear to exist.
coord leadingBit_Byte(const byte rankData) {
	return (coord)__builtin_clz((unsigned int)rankData);
}
*/

offset leadingBit_Bitboard(const bitboard b) {
	return (offset)__builtin_clzll((unsigned long long)rankData);
}

int populationCount(const bitboard b) {
	return __builtin_popcountll((unsigned long long)b);
}

bitboard getNextPiece(const bitboard b) {
	return 1 << leadingBit_Bitboard(b);
}


void printBB(const bitboard b) {
	int i;
	byte bit;
	bitboard flipped = flipBoardVert(b);
	for (i = 0; i < 64; i++) {
		
		// Mask, then shift down to equal 1 (or 0).
		bit = (flipped & (1ULL << i)) >> i;
		
		printf("%u", bit);

		if (i % 8 == 7) { // We just printed at the end of the line, so CR LF plox.
			printf("\n");
		}
		
	}
}

void printQB(const quadboard* qb) {
	int i;
	byte team,type0,type1,type2;
	for (i = 0; i < 64; i++) {
		
		// Mask, then shift down to equal 1 (or 0).
		team = (qb.team & (1ULL << i)) >> i;
		type0 = (qb.type0 & (1ULL << i)) >> i;
		type1 = (qb.type1 & (1ULL << i)) >> i;
		type2 = (qb.type2 & (1ULL << i)) >> i;
		
		byte type = (type0 << 2) | (type1 << 1) | type0;
		char c = ' ';
		switch(type) {
			case 0x001: c = 'P';
			case 0x010: c = 'R';
			case 0x011: c = 'N';
			case 0x100: c = 'B';
			case 0x101: c = 'Q';
			case 0x100: c = 'K';
		}

		if (team) {
			c += 'a' - 'A';
		}
		
		printf("%c", c);

		if (i % 8 == 7) { // We just printed at the end of the line, so CR LF plox.
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
	return  (team ? ~qbteam : qbteam)
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
	resetSquares(qb, to);
	qb->team  |= (qb->team  & from) ? to : 0;
	qb->type0 |= (qb->type0 & from) ? to : 0;
	qb->type1 |= (qb->type1 & from) ? to : 0;
	qb->type2 |= (qb->type2 & from) ? to : 0;
	resetSquares(qb, from);
}

bitboard setBitOffset(offset n) {
	return 1ULL << n;
}

/*
bitboard setBitXY(const coord x, const coord y) {
	return 1ULL << (x + (y * 8));
}
*/ 

/*
bitboard moveBitXY(bitboard val, int x, int y) {
	int offsetXY = x + (y * 8);
	if (offsetXY > 0) {
		return val << offsetXY;
	}
	else {
		return val >> (offsetXY * -1);
	}
}
*/

void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
}

/*
bitboard unionTeamBoard(const teamboard tb) {
	return tb.pawns | tb.rooks | tb.knights | tb.bishops | tb.queens | tb.king;
}
*/

bitboard getFrenemies(quadboard qb) {
	return qb.type0 | qb.type1 | qb.type2;
}


bitboard applySlidingAttackVector(	const bitboard piece, 
									const bitboard vector, 
									const bitboard frenemies, 
									const int direction) {

	bitboard attacks = 0ULL;

	// All vectors are fixed and so need to be relative 
	// to a moving cursor, not the stationary piece.
	bitboard cursor = piece;

	// Keep adding attacks along this vector
	// until the vector dies.
	do {

		// Create attack bitboard by shifting the piece
		// by the approprate vector offset.
		//
		bitboard attack = dir ? cursor << leadingBit_Bitboard(vector)
							  : cursor >> leadingBit_Bitboard(vector);
						  
									
		// Are we still on the board ?
		if (!attack || (dir ? attack % 8 == 0 : attack % 8 == 7)) { 

			// We ran off the edge of the board, kill the vector.
			return attacks;
			
		else {

			// Add to the attack "list" (actually a bitboard)
			attacks |= attack;
			
			// We are in attack list mode, so use the frenemies map
			// to see if our vector is dead
			if (frenemies & attack) {
				
				// We hit someone, end the vector
				return attacks;
			}
			
			// Move the cursor
			cursor = attack;
		}
	
	} while (1); // looks bad dunnit

	// UNREACHABLE
	
}

bitboard applySingleAttackVector(	const bitboard piece
									const bitboard vector, 
									const int direction) {

	// Create attack bitboard by shifting the piece
	// by the approprate vector offset.
	//
	bitboard attack = dir ? cursor << leadingBit_Bitboard(vector)
						  : cursor >> leadingBit_Bitboard(vector);
					  
								
	// Are we still on the board ?
	if (!attack || (dir ? attack % 8 == 0 : attack % 8 == 7)) { 

		// We ran off the edge of the board, nothing to add.
		return 0ULL;
		
	else {

		// Since we are in attack mode, just return this single attack
		// no matter what (as long as it's on the board, it's legit)
		return attack;

	}
	
}


bitboard generateAttacks(bitboard pieces, bitboard frenemies, bitboard positiveVectors, int slidingAttacks) {

	// This is the bitboard we build up and then return.
	bitboard attacks = 0ULL;

	//
	// Used to set initial value of cursor for each vector path
	// AND to remove the piece from the piece list in the piece loop.
	//	
	bitboard piece = getNextPiece(pieces);

	// There might be zero pieces, so check up front.
	while (piece) { 

		// Since we only have positive vectors, do everything twice,
		// but the second time, do it all backwards.
		int dir;
		for (dir = 0; dir < 2; dir ++) {

			// Create a new vector scratchlist.
			bitboard vectorList = positiveVectors;

			// This actually gets the next single bit mask, not the next piece.  
			// Rename the method to be generic
			bitboard vector = getNextPiece(vectorList);

			// Iterating over the vectors.
			do { 

				// Grab all the attacks along this vector/dir combo.
				if (slidingAttacks) {
					attacks |= applySlidingAttackVector(piece, vector, frenemies, dir);
				}
				else {
					attacks |= applySingleAttackVector(piece, vector, dir);
				}
				
				// Fetch the next vector (if any left).
				vector = getNextPiece(vectorList);
			
			// Have we run out of vectors?
			} while (vectorList);


		} // The forwards/backwards direction loop ends here
		
		
		// Remove the piece from the pieces "list"
		pieces &= ~piece;
		
		// Get the next piece, if any.
		piece = getNextPiece(pieces)
		
	} // exits when pieces "list" is empty


	// Viola !
	return attacks;
		
}

bitboard generateAttacks(board* b, int team) {

	bitboard frenemies = getFrenemies(b->quad);
	
	bitboard queens  = getQueens(b->quad, team);
	bitboard rooks   = getRooks(b->quad, team);
	bitboard bishops = getBishops(b->quad, team);
	bitboard knights = getKnights(b->quad, team);
	bitboard kings   = getKings(b->quad, team);

	return 
		  generateAttacks(queens,  frenemies, (1 << 7) | (1 << 8)  | (1 << 9)  | 1        , 1)
		| generateAttacks(kings,   frenemies, (1 << 7) | (1 << 8)  | (1 << 9)  | 1        , 0)
		| generateAttacks(rooks,   frenemies,            (1 << 8)              | 1        , 1)
		| generateAttacks(bishops, frenemies, (1 << 7)             | (1 << 9)             , 1)
		| generateAttacks(bishops, frenemies, (1 << 6) | (1 << 15) | (1 << 17) | (1 << 10), 0);
																
																	
}

/*

int populationCount(const bitboard* const object, const int sizeInBitBoards) {
	int i;
	int popCount = 0;
	for (i = 0; i < sizeInBitBoards; i++) {
		popCount += __builtin_popcountll((unsigned long long)(object[i]));
		printf("Popcount now at %d for input of %" PRIx64 "\n", popCount, object[i]);
	}
	return popCount;
}

*/

void clearQuadboard(quadboard* qb) {
	qb->teams = 0ULL;
	qb->type0 = 0ULL;
	qb->type1 = 0ULL;
	qb->type2 = 0ULL;
}

void initBoard(board* b) {
	
	setPawns(qb, 255 << (8 * 1), 0); // white
	setPawns(qb, 255 << (8 * 6), 1); // black

	setRooks(qb, 128 + 1,            0);
	setRooks(qb, 128 + 1 << (8 * 7), 1);

	setKnights(qb, 64 + 2, 0);
	setKnights(qb, 64 + 2 << (8 * 7), 1);

	setBishops(qb, 32 + 4, 0);
	setBishops(qb, 32 + 4 << (8 * 7), 1);

	setQueens(qb, 16, 0);
	setQueens(qb, 16 << (8 * 7), 1);
	
	setKings(qb, 8, 0);
	setKings(qb, 8 << (8 * 7), 0);

	qb->piecesMoved = 0;
	qb->whosTurn = 0; // WHITE
}


int main() {
	board b;
	initBoard(&b);
	
	printf("setBitXY 0,0 and 7,7: %" PRIx64 " %" PRIx64 "\n", setBitXY(0,0), setBitXY(7,7));
	
	int bbSize = sizeof(bitboard);
	int ullSize = sizeof(unsigned long long);
	int uiSize = sizeof(unsigned int);
	
	printf("Sizeof bitboard %d, sizeof unsigned long long %d, sizeof unsigned int %d\n", 
	       bbSize, ullSize, uiSize);
	
	printf("Number of pieces on the white teamboard %d\n", populationCount((bitboard*)&(b.white), sizeof(teamboard) / sizeof(bitboard) ));
	
	printf("Value of white.bishops: %" PRIx64 "\n", b.white.bishops);
	
	printf("Showing the board with only the white king on it.\n\n");
	printBB(b.white.king);

	printf("Showing the board with all the white pieces on it.\n\n");
	printBB(unionTeamBoard(&(b.white)));
	
}
