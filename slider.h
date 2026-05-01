#ifndef SLIDER_H
#define SLIDER_H

#include "bitboard.h"

// PEXT-bitboard slider attack lookup. Replaces the iterative ray-walk
// in singlePieceAttacks for rooks, bishops, and queens. Initialised
// once at process start via a __attribute__((constructor)).
//
// Caller provides the source square and full occupancy (both colours);
// the returned bitboard contains every square the slider reaches,
// INCLUDING the first blocker on each ray. The caller is responsible
// for masking out friendly pieces (`& ~friends`) before using it.

bitboard pextRookAttacks  (offset sq, bitboard occ);
bitboard pextBishopAttacks(offset sq, bitboard occ);
bitboard pextQueenAttacks (offset sq, bitboard occ);

#endif
