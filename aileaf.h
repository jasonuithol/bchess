#ifndef AILEAF_H
#define AILEAF_H

#include <stdint.h>

#include "bitboard.h"
#include "quadboard.h"
#include "umpire.h"

typedef uint32_t nodesCalculatedType;
typedef uint8_t depthType;

extern nodesCalculatedType nodesCalculated;

void displaySpinningPulse(void);
scoreType evaluateMaterial(const quadboard qb, const byte team);
scoreType evaluateMobility(const quadboard qb, const byte team);
scoreType analyseLeafNonTerminal(const quadboard qb, const byte team);
scoreType analyseLeafTerminal(const board* const b, const byte scoringTeam, const depthType depth);

#endif
