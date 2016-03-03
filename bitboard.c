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
void printBB(const bitboard b) {

	bitboard flipped = flipBoardVert(b);

	for (int i = 0; i < 64; i++) {
		
		byte bit;

		// Mask, then shift down to equal 1 (or 0).
		bit = (flipped & (1ULL << i)) >> i;
		
		printf("%u", bit);

		if (i % 8 == 7) { // We just printed at the end of the line, so CR LF plox.
			printf("\n");
		}
		
	}
}

offset trailingBit_Bitboard(const bitboard b) {
/*	
	unsigned long long nativeInputValue = (unsigned long long)b;
	int nativeReturnValue = __builtin_ctzll(nativeInputValue);
	offset actualReturnValue = (offset)nativeReturnValue;
	printf("Trailing bit calculation[actualin 0x%" PRIx64 ", nativein 0x%llx, nativeout %d, actualout %u\n"
	,b, nativeInputValue, nativeReturnValue, actualReturnValue); 
	printf("Printing bitboard for native input\n");
	printBB(b);
	return actualReturnValue;
*/
	return (offset)__builtin_ctzll((unsigned long long)b);
}

int populationCount(const bitboard b) {
	return __builtin_popcountll((unsigned long long)b);
}

bitboard getNextPiece(const bitboard b) {
	return b ? 1ULL << trailingBit_Bitboard(b) : 0ULL;
}




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
			case 1: c = 'P'; break;		// 001
			case 2: c = 'R'; break;		// 010
			case 3: c = 'N'; break;		// 011
			case 4: c = 'B'; break;		// 100
			case 5: c = 'Q'; break;		// 101
			case 6: c = 'K'; break;		// 110
			default: c = ' '; break;	// 000, (and in theory, >= 111)
		}

		if (team && c != '?') {
			// If BLACK, then decapitalise in order to tell the difference
			// between the two teams.
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
									const bitboard softBlockers, 
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
		printf("Applying sliding attack vector to cursor in direction %s, printing cursor and vector bitboards\n", direction ? "DOWN" : "UP");
		printBB(cursor);
		printf("\n");
		printBB(vector);
		
		bitboard attack = direction ? cursor >> trailingBit_Bitboard(vector)  // DOWN
							        : cursor << trailingBit_Bitboard(vector); // UP
						  

		printf("Showing calculated attack destination\n");
		printBB(attack);
		
		// DEBUG ONLY
		if (attack == 0) {
			printf("Obtained zero from 0x%" PRIx64 " << %u\n", cursor, trailingBit_Bitboard(vector));
		}
									
		// This is the difference in the fileIx of the attack and cursor positions.
		// Normally it's 1 for sliding pieces.
		// However, if the attack wrapped around one of the vertical sides,
		// then the modulo distance will be 7.
		//
		int moduloDistance = abs((trailingBit_Bitboard(attack) % 8) - (trailingBit_Bitboard(cursor) % 8));

		// Are we still on the board ?
		if (!attack || moduloDistance > 2) { // Anything larger than 2 is out.
											 // We say 2 to keep in line with
											 // single attack calculation.

			printf("We ran off the edge of the board, not adding attack to list.  Modulo distance %d.  Exiting sliding vector\n", moduloDistance);

			// We ran off the edge of the board, kill the vector.
			return attacks;
			
		}	
		else {

			// Add to the attack "list" (actually a bitboard)
			attacks |= attack;
			
			// Hitting a soft block means we still add the attack to the list,
			// but then kill the vector immediately after that.
			if (softBlockers & attack) {

				printf("We ran into a soft blocker, still adding attack to list.  Exiting sliding vector\n");
				
				// We hit someone, end the vector
				return attacks;
			}
			
			// Move the cursor
			cursor = attack;
		}
	
	} while (1); // looks bad dunnit

	// UNREACHABLE
	
}

bitboard applySingleAttackVector(	const bitboard cursor,
									const bitboard vector, 
									const int direction) {

	// Create attack bitboard by shifting the piece
	// by the approprate vector offset.
	//
	printf("Applying single attack vector to cursor in direction %d (0=UP,1=DOWN), printing piece bitboard\n", direction);
	printBB(cursor);
	
	bitboard attack = direction ? cursor >> trailingBit_Bitboard(vector)  // DOWN
						        : cursor << trailingBit_Bitboard(vector); // UP

					  
	printf("Showing calculated attack destination\n");
	printBB(attack);

	// This is the difference in the fileIx of the attack and cursor positions.
	// Normally it's 1 for kings, 1-2 for knights.
	// However, if the attack wrapped around one of the vertical sides,
	// then the modulo distance will be 6-7.
	//
	int moduloDistance = abs((trailingBit_Bitboard(attack) % 8) - (trailingBit_Bitboard(cursor) % 8));

	// Are we still on the board ?
	if (!attack || moduloDistance > 2) { // Anything larger than 2 is out.
	
		// We ran off the edge of the board, nothing to add.
		return 0ULL;
		
	}	
	else {

		// Since we are in attack mode, just return this single attack
		// no matter what (as long as it's on the board, it's legit)
		return attack;

	}
	
}

#define ATTACKMODE_SINGLE 	(0)
#define ATTACKMODE_SLIDING 	(1)
#define ATTACKMODE_PAWN 	(2)

bitboard pieceAttacks(bitboard pieces, bitboard softBlockers, bitboard positiveVectors, int attackMode) {

	// This is the bitboard we build up and then return.
	bitboard attacks = 0ULL;

	//
	// Used to set initial value of cursor for each vector path
	// AND to remove the piece from the piece list in the piece loop.
	//	
	bitboard piece = getNextPiece(pieces);

	// There might be zero pieces, so check up front.
	while (pieces) { 

		printf("Entering new piece loop iteration, showing piece bitboard\n");
		
		printBB(piece);

		// Since we only have positive vectors, do everything twice,
		// but the second time, do it all backwards.
		//
		// 0=UP, 1=DOWN
		//
		for (int dir = 0; dir < 2; dir ++) {

			// Create a new vector scratchlist.
			bitboard vectorList = positiveVectors;

			// This actually gets the next single bit mask, not the next piece.  
			// Rename the method to be generic
			bitboard vector = getNextPiece(vectorList);
			
			printf("Pulled new vector off vectorList, showing vector bitboard\n");
			printBB(vector);

			// Iterating over the vectors.
			do { 

				// Grab all the attacks along this vector/dir combo.
				if (attackMode == ATTACKMODE_SLIDING) {
					printf("Applying sliding attack vector...");
					attacks |= applySlidingAttackVector(piece, vector, softBlockers, dir);
					printf("DONE\n");
				}
				else {
					// This applies to SINGLE and PAWN attack modes.
					printf("Applying single attack vector...");
					attacks |= applySingleAttackVector(piece, vector, dir);
					printf("DONE\n");
				}
				
				printf("Showing accumulated attacks bitboard\n");
				printBB(attacks);
				
				// Remove vector from vectorList
				vectorList &= ~vector;
				
				// Fetch the next vector (if any left).
				vector = getNextPiece(vectorList);
			
			// Have we run out of vectors?
			} while (vectorList);

			// Don't do the backwards direction for pawns.
			if (attackMode == ATTACKMODE_PAWN) {
				break;
			}

		} // The forwards/backwards direction loop ends here
		
		// Remove the piece from the pieces "list"
		pieces &= ~piece;
		
		// Get the next piece, if any.
		piece = getNextPiece(pieces);
		
	} // exits when pieces "list" is empty


	// Viola !
	return attacks;
		
}

// These are positive-only attack vectors for all pieces except KNIGHT.
const bitboard nw = 1ULL << 9;
const bitboard n  = 1ULL << 8;
const bitboard ne = 1ULL << 7;
const bitboard w  = 1ULL << 1;

// These are positive-only attack vectors for KNIGHT.
const bitboard nww = 1ULL << 6;
const bitboard nnw = 1ULL << 15;
const bitboard nne = 1ULL << 17;
const bitboard nee = 1ULL << 10;

bitboard generateAttacks(board* b, int team) {

	bitboard softBlockers = getFrenemies(b->quad);
	
	return
		// SLIDING PIECES
		  pieceAttacks(getQueens(b->quad, team),  softBlockers, nw | n | ne | w, ATTACKMODE_SLIDING)
		| pieceAttacks(getRooks(b->quad, team),   softBlockers,      n      | w, ATTACKMODE_SLIDING)
		| pieceAttacks(getBishops(b->quad, team), softBlockers, nw     | ne    , ATTACKMODE_SLIDING)
		
		// SINGLE AND PAWN PIECES
		| pieceAttacks(getKings(b->quad, team),   softBlockers, nw | n | ne | w, ATTACKMODE_SINGLE)
		| pieceAttacks(getPawns(b->quad, team),   softBlockers, nw     | ne    , ATTACKMODE_PAWN)
 		| pieceAttacks(getKnights(b->quad, team), softBlockers, nww | nnw |nne | nee, ATTACKMODE_SINGLE);
}

bitboard generateMoves(board* b, int team) {
	
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


void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
}

void initBoard(board* b) {
	
	quadboard* qb = &(b->quad);
	
	clearBoard(b);
	
	addPawns(qb, 255ULL << (8 * 1), 0); // white
	addPawns(qb, 255ULL << (8 * 6), 1); // black

	addRooks(qb, (128ULL + 1),            0);
	addRooks(qb, (128ULL + 1) << (8 * 7), 1);

	addKnights(qb, (64ULL + 2), 0);
	addKnights(qb, (64ULL + 2) << (8 * 7), 1);

	addBishops(qb, (32ULL + 4), 0);
	addBishops(qb, (32ULL + 4) << (8 * 7), 1);

	addKings(qb, 16ULL, 0);
	addKings(qb, 16ULL << (8 * 7), 1);

	addQueens(qb, 8ULL, 0);
	addQueens(qb, 8ULL << (8 * 7), 1);
	

	b->piecesMoved = 0;
	b->whosTurn = 0; // WHITE
}


int main() {
	board b;
	initBoard(&b);
	
//	printf("setBitXY 0,0 and 7,7: %" PRIx64 " %" PRIx64 "\n", setBitXY(0,0), setBitXY(7,7));
	
//	int bbSize = sizeof(bitboard);
//	int ullSize = sizeof(unsigned long long);
//	int uiSize = sizeof(unsigned int);	
//	printf("Sizeof bitboard %d, sizeof unsigned long long %d, sizeof unsigned int %d\n", 
//	       bbSize, ullSize, uiSize);
	
//	printf("Number of pieces on the white teamboard %d\n", populationCount((bitboard*)&(b.white), sizeof(teamboard) / sizeof(bitboard) ));
	
//	printf("Value of white.bishops: %" PRIx64 "\n", b.white.bishops);
	
	printf("8ULL << 7 = %lld\n", 8ULL << 7);
	bitboard testb = 8;
	offset testo = 7;
	bitboard result = testb << testo;
	printf("testb << test0 = %" PRId64 "\n", result);
	printBB(result);
	
	
	printf("Showing the whole board\n\n");
	printQB(b.quad);

	bitboard whiteAttacks = generateAttacks(&b, 0);
	
	printf("Print attack matrix for white\n\n");
	printBB(whiteAttacks);
	
//	printf("Showing the board with all the white pieces on it.\n\n");
//	printBB(unionTeamBoard(&(b.white)));
	
}
