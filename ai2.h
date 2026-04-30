#ifndef AI2_H
#define AI2_H

#include "umpire.h"
#include "aileaf.h"

scoreType getBestMove(analysisMove* const bestMove,
                      const board* const loopDetect,
                      const board* const b,
                      const byte scoringTeam,
                      const depthType aiStrength,
                      const depthType depth,
                      scoreType alpha,
                      scoreType beta);

#endif
