#ifndef SAVEGAME_H
#define SAVEGAME_H

#include "bitboard.h"
#include "umpire.h"

typedef struct {
    int turn;
    board boards[5];
    byte current;
    byte next;
    byte loopDetectIx;
    byte loopCount;
} gameContext;

void newGame(gameContext* game);
void load(gameContext* game);
void save(gameContext* game);

#endif
