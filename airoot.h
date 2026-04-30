#ifndef AIROOT_H
#define AIROOT_H

#include "umpire.h"

void aiMove(const board* const current, board* const next, const board* const loopDetectPtr, const int turnNumber);

#endif
