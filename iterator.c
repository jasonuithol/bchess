#include "iterator.h"
#include "bitboard.h"

iterator getNextItem(iterator i) {
    i.item = i.list ? 1ULL << trailingBit_Bitboard(i.list) : 0ULL;
    i.list &= ~i.item;
    return i;
}
