typedef uint8_t  byte;
typedef uint8_t  offset;
typedef int16_t  relativeOffset;

typedef uint64_t bitboard;


// Up and down go swapsies.
bitboard flipBoardVert(bitboard b) {
	return (bitboard)__builtin_bswap64((uint64_t)b);
}

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
	return (offset)__builtin_ctzll((unsigned long long)b);
}

int populationCount(const bitboard b) {
	return __builtin_popcountll((unsigned long long)b);
}

