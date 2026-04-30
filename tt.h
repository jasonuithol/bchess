#ifndef TT_H
#define TT_H

#include <stdint.h>

#include "bitboard.h"
#include "umpire.h"
#include "aileaf.h"

typedef enum {
    TT_NONE  = 0,
    TT_EXACT = 1,
    TT_LOWER = 2,
    TT_UPPER = 3,
} ttBound;

typedef struct {
    int hit;            // key matched an entry
    int usable;         // hit AND (depth, bound) allow score reuse under (alpha, beta)
    scoreType score;
    bitboard bestFrom;
    bitboard bestTo;
} ttResult;

void ttInit(void);
void ttClear(void);
void ttNewSearch(void);

uint64_t computeZobristHash(const board* const b);

// Probe at this depth/window. The result's bestFrom/bestTo are filled
// whenever the key matches, even if score reuse isn't permitted — the move
// can still seed move ordering.
ttResult ttProbe(uint64_t key, depthType depthRemaining, scoreType alpha, scoreType beta);

void ttStore(uint64_t key,
             depthType depthRemaining,
             scoreType score,
             ttBound bound,
             bitboard bestFrom,
             bitboard bestTo);

#endif
