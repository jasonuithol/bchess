#include <stdint.h>
#include <stdio.h>

#include "bitboard.h"
#include "logging.h"

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

offset trailingBit_Bitboard(const bitboard b) {
    return (offset)__builtin_ctzll((unsigned long long)b);
}

int populationCount(const bitboard b) {
    return __builtin_popcountll((unsigned long long)b);
}

offset getFile(const bitboard b) {
    return trailingBit_Bitboard(b) % 8;
}

offset getRank(const bitboard b) {
    return trailingBit_Bitboard(b) / 8;
}

void printSquare(bitboard square) {
    print("%c%c", (7 - getFile(square)) + 'a', getRank(square) + '1');
}
