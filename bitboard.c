// ==================================================================
//
// bitboard.c
//
// Author: 		Jason Uithol
// Copyright: 	2016
//
// State Use: N/A
//
// ==================================================================

typedef uint8_t  byte;
typedef uint8_t  offset;
typedef int16_t  relativeOffset;

typedef uint64_t bitboard;


// Up and down go swapsies.
bitboard flipBoardVert(bitboard b) {
	return (bitboard)__builtin_bswap64((uint64_t)b);
}

void printBB(const bitboard b) {

	for (int i = 63; i >= 0; i--) {
		
		byte bit;

		// Mask, then shift down to equal 1 (or 0).
		bit = (b & (1ULL << i)) >> i;
		
		printf("%u", bit);

		if (i % 8 == 0) { // We just printed at the end of the line, so CR LF plox.
			printf("\n");
		}
		
	}
}

inline offset trailingBit_Bitboard(const bitboard b) {
	return (offset)__builtin_ctzll((unsigned long long)b);
}

inline int populationCount(const bitboard b) {
	return __builtin_popcountll((unsigned long long)b);
}

inline offset getFile(const bitboard b) {
	return trailingBit_Bitboard(b) % 8;
}

inline offset getRank(const bitboard b) {
	return trailingBit_Bitboard(b) / 8;
}

void printSquare(bitboard square) {
	print("%c%c", (7 - getFile(square)) + 'a', getRank(square) + '1');
}

#define toBitboard(file,rank) (1ULL << ((7 - (file - 'a')) + ((rank - '1') * 8)))


