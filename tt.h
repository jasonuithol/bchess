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

// Zobrist random tables. Exposed so umpire.c's applyMove can XOR-update
// b->hash incrementally without recomputing from scratch at every node.
// Indices match the layout in tt.c. Lazily initialised — anyone reading
// these should first ensure zobInit has run; computeZobristHash and
// ttInit both guarantee that.
extern uint64_t zobPiece[64][14];
extern uint64_t zobSide;
extern uint64_t zobCastle[256];
extern uint64_t zobEnPassant[9];

// Recompute the full hash from scratch. Used at boundaries: parseFen,
// initBoard, post-spawn fix-up, and as a debug oracle for incremental.
uint64_t computeZobristHash(const board* const b);

// Idempotent. Callers that touch zobPiece/etc. before ttInit must call
// this first; computeZobristHash also calls it lazily.
void zobristEnsureInit(void);

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
