#ifndef AI2_H
#define AI2_H

#include "umpire.h"
#include "aileaf.h"

// When the search detects that the configured deadline has passed it
// flips this flag and unwinds. Callers must check it after getBestMove
// returns and discard the result if it is set (the partially-explored
// iteration cannot be trusted).
extern volatile int searchAborted;

// Arm an abort deadline expressed in absolute CLOCK_MONOTONIC ms.
// Pass 0 (or call clearSearchDeadline) to disable time-based aborts.
void setSearchDeadlineMs(long absoluteMs);
void clearSearchDeadline(void);

// Trip the abort flag manually — used when the GUI sends "stop" while
// the search is running.
void requestSearchAbort(void);

scoreType getBestMove(analysisMove* const bestMove,
                      const board* const loopDetect,
                      board* const b,
                      const byte scoringTeam,
                      const depthType aiStrength,
                      const depthType depth,
                      scoreType alpha,
                      scoreType beta);

#endif
