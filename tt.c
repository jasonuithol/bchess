// ==================================================================
//
// tt.c
//
// Zobrist hashing + transposition table.
//
// Layout:
//   - 64 squares x 14 piece codes (some unused) of zobrist keys
//   - 1 side-to-move key XORed when black is to move
//   - 256 castling keys indexed by piecesMoved (the persistent has-moved
//     bitset; the transient currentCastlingRights overlay is a pure
//     function of the rest of the board, so we don't hash it)
//
// TT is a single-bucket open-addressed table indexed by key & MASK.
// Replacement: same-key always wins; otherwise, prefer the entry with
// greater remaining-depth, or any entry from an older search generation.
//
// ==================================================================

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "tt.h"
#include "bitboard.h"
#include "logging.h"
#include "quadboard.h"
#include "umpire.h"


static uint64_t zobPiece[64][14];
static uint64_t zobSide;
static uint64_t zobCastle[256];
static int zobInitDone = 0;

#define TT_SIZE_LOG2 20
#define TT_SIZE      (1ULL << TT_SIZE_LOG2)
#define TT_MASK      (TT_SIZE - 1)

typedef struct {
    uint64_t  key;
    bitboard  bestFrom;
    bitboard  bestTo;
    scoreType score;
    uint8_t   depth;       // remaining depth at time of store
    uint8_t   bound;       // ttBound
    uint8_t   generation;
} ttEntry;

static ttEntry* tt = NULL;
static uint8_t  ttGen = 0;


static uint64_t splitmix64(uint64_t* state) {
    uint64_t z = (*state += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

static void zobInit(void) {
    uint64_t state = 0xC0FFEE12345BEEFULL;
    for (int sq = 0; sq < 64; sq++) {
        for (int p = 0; p < 14; p++) {
            zobPiece[sq][p] = splitmix64(&state);
        }
    }
    zobSide = splitmix64(&state);
    for (int i = 0; i < 256; i++) {
        zobCastle[i] = splitmix64(&state);
    }
    zobInitDone = 1;
}


uint64_t computeZobristHash(const board* const b) {
    uint64_t h = 0;
    bitboard occ = getAllPieces(b->quad);
    while (occ) {
        offset sq = trailingBit_Bitboard(occ);
        bitboard sqBit = 1ULL << sq;
        byte type = getType(b->quad, sq);
        byte team = getTeam(b->quad, sq);
        h ^= zobPiece[sq][type | team];
        occ &= ~sqBit;
    }
    if (b->whosTurn == BLACK) {
        h ^= zobSide;
    }
    h ^= zobCastle[b->piecesMoved];
    return h;
}


void ttInit(void) {
    if (!zobInitDone) {
        zobInit();
    }
    if (tt == NULL) {
        tt = (ttEntry*)calloc(TT_SIZE, sizeof(ttEntry));
        if (!tt) {
            error("Failed to allocate transposition table\n");
        }
    }
    ttGen = 0;
}

void ttClear(void) {
    if (tt) {
        memset(tt, 0, TT_SIZE * sizeof(ttEntry));
    }
    ttGen = 0;
}

// This engine uses fixed-perspective (scoringTeam) scoring rather than
// negamax, so TT entries stored during a search for one team have the
// opposite sign from entries we'd want next search. Easiest correct fix:
// wipe the table between searches. Within a search, transpositions still
// pay off — that's where most of the win is for a depth-6 engine.
void ttNewSearch(void) {
    ttClear();
    ttGen++;
}


// Mate scores are encoded as (mateValue - distance), so they're path-relative.
// We don't reuse mate scores across TT entries; treat them as always-unusable
// to avoid the depth-adjustment complexity. The move slot is still useful.
#define MATE_THRESHOLD 9000

ttResult ttProbe(uint64_t key, depthType depthRemaining, scoreType alpha, scoreType beta) {
    ttResult r;
    r.hit = 0;
    r.usable = 0;
    r.score = 0;
    r.bestFrom = 0;
    r.bestTo = 0;

    if (!tt) return r;

    const ttEntry* e = &tt[key & TT_MASK];
    if (e->key != key || e->bound == TT_NONE) {
        return r;
    }

    r.hit = 1;
    r.bestFrom = e->bestFrom;
    r.bestTo = e->bestTo;

    if (e->depth < depthRemaining) {
        return r;
    }
    if (e->score > MATE_THRESHOLD || e->score < -MATE_THRESHOLD) {
        return r;
    }

    switch ((ttBound)e->bound) {
        case TT_EXACT:
            r.score = e->score;
            r.usable = 1;
            break;
        case TT_LOWER:
            if (e->score >= beta) {
                r.score = e->score;
                r.usable = 1;
            }
            break;
        case TT_UPPER:
            if (e->score <= alpha) {
                r.score = e->score;
                r.usable = 1;
            }
            break;
        default:
            break;
    }
    return r;
}


void ttStore(uint64_t key,
             depthType depthRemaining,
             scoreType score,
             ttBound bound,
             bitboard bestFrom,
             bitboard bestTo) {
    if (!tt) return;

    ttEntry* e = &tt[key & TT_MASK];

    // Replace policy: always overwrite stale generations; otherwise prefer
    // the entry searched to greater depth.
    if (e->generation != ttGen
        || e->key == key
        || depthRemaining >= e->depth) {

        e->key = key;
        e->depth = depthRemaining;
        e->score = score;
        e->bound = (uint8_t)bound;
        e->bestFrom = bestFrom;
        e->bestTo = bestTo;
        e->generation = ttGen;
    }
}
