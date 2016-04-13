// ==================================================================
//
// iterator.c
//
// Author:      Jason Uithol
// Copyright:   2016
//
// State Use: N/A
//
// ==================================================================

typedef union {
    // Allows for constructors in form { bitboard, bitboard }
    union {
        bitboard data[2];
    };
    
    // Friendly way to access items. Relies on compiler aligning the fields deterministically.
    struct {
        bitboard item;
        bitboard list;
    };
} iterator;

inline iterator getNextItem(iterator i) {
    i.item = i.list ? 1ULL << trailingBit_Bitboard(i.list) : 0ULL;
    i.list &= ~i.item;
    return i;
}

#define newIterator(x) { .item = 0ULL, .list = x }; 
