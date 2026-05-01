// ==================================================================
//
// slider.c
//
// PEXT-bitboard slider attack lookup tables.
//
// Replaces the per-piece ray-walk in singlePieceAttacks for rooks,
// bishops, and queens. The ray-walk did up to 7 iterations per ray,
// 4 rays per rook, 8 per queen — easily 50+ cycles per piece per leaf
// eval. PEXT does it in a single instruction + a single L1/L2 load:
// roughly 5x speedup on the hot path on a Zen4 (PEXT is 3-cycle
// latency from Zen3 onward).
//
// ==================================================================

#include <immintrin.h>  // _pext_u64, _pdep_u64
#include <stdint.h>
#include <string.h>

#include "slider.h"
#include "attacks.h"
#include "bitboard.h"

// --- Layout reminder ------------------------------------------------
// In bchess, bit 0 = h1, bit 7 = a1, bit 8 = h2, ..., bit 63 = a8.
// rank = sq / 8 (0 = rank 1).
// file = sq % 8 (0 = h-file, 7 = a-file).
// "North" shifts bits left by 8 (toward higher ranks).
// "West-in-chess-letters" (toward a-file) shifts left by 1 (since the
//   a-file lives at higher bit positions within a rank than the h-file).
// --------------------------------------------------------------------

static bitboard rookMask  [64];
static bitboard bishopMask[64];

// Worst-case relevant-bit count: rook 12, bishop 9.
static bitboard rookAttacksTable  [64][1 << 12];
static bitboard bishopAttacksTable[64][1 << 9];

// Build the "relevant blockers" mask for a slider at sq: every square
// along the slider's rays whose occupancy changes its attack pattern.
// Squares at the end of each ray (the last square on the board in
// that direction) are NOT relevant — there's nothing beyond them, so
// flipping their occupancy doesn't affect what the slider reaches.
static bitboard buildRookMask(int sq) {
    bitboard mask = 0;
    int rank = sq / 8;
    int file = sq % 8;

    for (int r = rank + 1; r <= 6; r++) mask |= 1ULL << (r * 8 + file);
    for (int r = rank - 1; r >= 1; r--) mask |= 1ULL << (r * 8 + file);
    for (int f = file + 1; f <= 6; f++) mask |= 1ULL << (rank * 8 + f);
    for (int f = file - 1; f >= 1; f--) mask |= 1ULL << (rank * 8 + f);

    return mask;
}

static bitboard buildBishopMask(int sq) {
    bitboard mask = 0;
    int rank = sq / 8;
    int file = sq % 8;

    for (int r = rank + 1, f = file + 1; r <= 6 && f <= 6; r++, f++)
        mask |= 1ULL << (r * 8 + f);
    for (int r = rank + 1, f = file - 1; r <= 6 && f >= 1; r++, f--)
        mask |= 1ULL << (r * 8 + f);
    for (int r = rank - 1, f = file + 1; r >= 1 && f <= 6; r--, f++)
        mask |= 1ULL << (r * 8 + f);
    for (int r = rank - 1, f = file - 1; r >= 1 && f >= 1; r--, f--)
        mask |= 1ULL << (r * 8 + f);

    return mask;
}

// Reference oracle: ray-walk a single direction from `piece` against
// occupancy `occ` (every occ bit treated as a soft blocker — the ray
// stops AT the blocker, includes it in the result). Direction encoding
// matches applySlidingAttackVector: 0 = bit-shift left, 1 = right.
static bitboard rayAttacks(bitboard piece, bitboard vector, bitboard occ, int dir) {
    return applySlidingAttackVector(piece, vector, occ, 0, (byte)dir);
}

static bitboard rookAttacksSlow(int sq, bitboard occ) {
    const bitboard piece = 1ULL << sq;
    bitboard a = 0;
    a |= rayAttacks(piece, 1ULL << 8, occ, 0);  // north
    a |= rayAttacks(piece, 1ULL << 8, occ, 1);  // south
    a |= rayAttacks(piece, 1ULL << 1, occ, 0);  // toward a-file
    a |= rayAttacks(piece, 1ULL << 1, occ, 1);  // toward h-file
    return a;
}

static bitboard bishopAttacksSlow(int sq, bitboard occ) {
    const bitboard piece = 1ULL << sq;
    bitboard a = 0;
    a |= rayAttacks(piece, 1ULL << 9, occ, 0);  // north + a-file
    a |= rayAttacks(piece, 1ULL << 7, occ, 0);  // north + h-file
    a |= rayAttacks(piece, 1ULL << 9, occ, 1);  // south + h-file (mirror)
    a |= rayAttacks(piece, 1ULL << 7, occ, 1);  // south + a-file (mirror)
    return a;
}

static int popcount64(uint64_t x) { return __builtin_popcountll(x); }

__attribute__((constructor))
static void sliderInit(void) {
    for (int sq = 0; sq < 64; sq++) {
        rookMask  [sq] = buildRookMask  (sq);
        bishopMask[sq] = buildBishopMask(sq);

        // Enumerate every distinct blocker pattern on the relevant
        // squares and pre-compute the attack bitboard for it. PDEP
        // takes the low N bits of an integer index and spreads them
        // into the positions specified by the mask, giving us the
        // 1:1 mapping with PEXT used at lookup time.
        const int rb = popcount64(rookMask[sq]);
        for (int i = 0; i < (1 << rb); i++) {
            const bitboard occ = _pdep_u64((uint64_t)i, rookMask[sq]);
            rookAttacksTable[sq][i] = rookAttacksSlow(sq, occ);
        }

        const int bb = popcount64(bishopMask[sq]);
        for (int i = 0; i < (1 << bb); i++) {
            const bitboard occ = _pdep_u64((uint64_t)i, bishopMask[sq]);
            bishopAttacksTable[sq][i] = bishopAttacksSlow(sq, occ);
        }
    }
}

bitboard pextRookAttacks(offset sq, bitboard occ) {
    const uint64_t idx = _pext_u64(occ, rookMask[sq]);
    return rookAttacksTable[sq][idx];
}

bitboard pextBishopAttacks(offset sq, bitboard occ) {
    const uint64_t idx = _pext_u64(occ, bishopMask[sq]);
    return bishopAttacksTable[sq][idx];
}

bitboard pextQueenAttacks(offset sq, bitboard occ) {
    return pextRookAttacks(sq, occ) | pextBishopAttacks(sq, occ);
}
