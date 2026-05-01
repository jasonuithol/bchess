#ifndef UMPIRE_H
#define UMPIRE_H

#include <stdint.h>

#include "bitboard.h"
#include "quadboard.h"

#define BOARD_LEGAL     (1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL    (0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)

typedef struct {
    quadboard quad;
    // The square an en-passant capturing pawn would LAND on (the empty
    // square the just-moved opponent pawn skipped over). 0 if no e.p.
    // is available — i.e., the previous move wasn't a pawn double-push.
    bitboard enPassantTarget;
    byte currentCastlingRights;
    byte piecesMoved;
    byte whosTurn;
} board;

typedef int16_t scoreType;

typedef struct {
    bitboard from;
    bitboard to;
    scoreType score;
    byte promoteTo;
} analysisMove;

#define ANALYSIS_SIZE (255)

typedef struct {
    analysisMove items[ANALYSIS_SIZE];
    byte ix;
} analysisList;

void clearBoard(board* const b);
byte isPawnPromotable(const bitboard piece);
byte isKingChecked(const quadboard qb, byte team);
byte determineEndOfGameState(const board* const b);
void computeCurrentCastlingRights(board* const b);
byte spawnLeafBoard(const board* const old, board* const new, const bitboard from, const bitboard to, const byte promoteTo);
byte spawnFullBoard(const board* const old, board* const new, const bitboard from, const bitboard to, const byte promoteTo);
byte addMoveIfLegal(analysisList* const list, const board* const old, const bitboard from, const bitboard to, const byte promoteTo, const byte leafMode);
void generateLegalMoveList(const board* const b, analysisList* const moveList, const byte leafMode);
void printMove(const analysisMove move);
void printMoveList(const analysisList* const moveList);
void printAllowedMoves(const board* const b);
byte makeMove(const board* const old, board* const new, const analysisMove* const move);
byte detectCheckmate(const board* const b);
void initBoard(board* const b);

#endif
