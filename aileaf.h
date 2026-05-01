#ifndef AILEAF_H
#define AILEAF_H

#include <stdatomic.h>
#include <stdint.h>

#include "bitboard.h"
#include "quadboard.h"
#include "umpire.h"

typedef uint32_t nodesCalculatedType;
typedef uint8_t depthType;

// Atomic so multiple search threads can each contribute to the visit
// count without racing. Relaxed ordering is enough — we only read it
// after the search has finished, for the info-line node count.
extern _Atomic nodesCalculatedType nodesCalculated;

// When non-zero, displaySpinningPulse skips its terminal animation.
// UCI mode sets this so the spinner doesn't end up wedged into the
// "bestmove" line that GUIs/match runners parse.
extern int suppressSpinnerOutput;

void displaySpinningPulse(void);
scoreType evaluateMaterial(const quadboard qb, const byte team);
scoreType evaluateMobility(const quadboard qb, const byte team);
scoreType analyseLeafNonTerminal(const quadboard qb, const byte team);
scoreType analyseLeafTerminal(const board* const b, const byte scoringTeam, const depthType depth);

#endif
