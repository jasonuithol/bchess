#ifndef ITERATOR_H
#define ITERATOR_H

#include "bitboard.h"

typedef union {
    union {
        bitboard data[2];
    };
    struct {
        bitboard item;
        bitboard list;
    };
} iterator;

iterator getNextItem(iterator i);

#define newIterator(x) { .item = 0ULL, .list = x };

#endif
