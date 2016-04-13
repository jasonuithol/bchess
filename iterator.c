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

typedef struct {
    bitboard item;
    bitboard list;
} iterator;

iterator getNextItem(iterator i) {

    // Remove the last item from the list (NOP if item not yet populated)
    i.list ^= i.item;

    // Mask out the next set bit in items (will be 0 if nothing in list)
    i.item = i.list & (1ULL << trailingBit_Bitboard(i.list));
    
    return i;
}

#define newIterator(x) { .item = 0ULL, .list = x }; 
