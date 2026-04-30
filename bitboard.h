#ifndef BITBOARD_H
#define BITBOARD_H

#include <stdint.h>
#include <stdio.h>

#include "logging.h"

typedef uint8_t  byte;
typedef uint8_t  offset;
typedef int16_t  relativeOffset;
typedef uint64_t bitboard;

bitboard flipBoardVert(bitboard b);
void printBB(const bitboard b);
offset trailingBit_Bitboard(const bitboard b);
int populationCount(const bitboard b);
offset getFile(const bitboard b);
offset getRank(const bitboard b);
void printSquare(bitboard square);

#define toBitboard(file,rank) (1ULL << ((7 - (file - 'a')) + ((rank - '1') * 8)))

#endif
