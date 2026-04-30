#ifndef MOVEORDERING_H
#define MOVEORDERING_H

#include "bitboard.h"
#include "quadboard.h"
#include "umpire.h"
#include "aileaf.h"

void clearKillers(void);
void clearHistory(void);
void addKiller(const bitboard from, const bitboard to, const depthType depth);
void updateHistory(const byte pieceType, const bitboard to, const depthType depth);
int getHistoryScore(const byte pieceType, const bitboard to);
byte isKiller(const bitboard from, const bitboard to, const depthType depth);
int scoreMove(const analysisMove* move, const quadboard* prevBoard, const depthType depth, bitboard hashFrom, bitboard hashTo);
void sortMoves(analysisList* moveList, const quadboard* prevBoard, const depthType depth, bitboard hashFrom, bitboard hashTo);

#endif
