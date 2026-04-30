#ifndef QUADBOARD_H
#define QUADBOARD_H

#include "bitboard.h"

#define WHITE ((byte)0)
#define BLACK ((byte)1)

#define PAWN    ((byte)2)
#define ROOK    ((byte)4)
#define KNIGHT  ((byte)6)
#define BISHOP  ((byte)8)
#define QUEEN   ((byte)10)
#define KING    ((byte)12)

#define UNICODESET_SOLID (0)
#define UNICODESET_GHOST (1)

typedef struct {
    bitboard type0;
    bitboard type1;
    bitboard type2;
    bitboard team;
} quadboard;

byte opponentOf(byte team);
byte areEqualQB(quadboard a, quadboard b);
void printByte(const byte v);
byte getTeam(const quadboard qb, offset i);
byte getType(const quadboard qb, offset i);
void printQB(const quadboard qb);
void printTeamColor(const byte team);
void printResetColors(void);
void printPieceUnicode(const byte type, const byte team, const int setToUse);
void printQBUnicode(const quadboard qb);
void addPieces(quadboard* const qb, const bitboard pieces, const byte pieceType);
bitboard getPieces(const quadboard qb, const byte pieceType);
void resetSquares(quadboard* const qb, const bitboard squares);
void moveSquare(quadboard* const qb, const bitboard from, const bitboard to);
bitboard getAllPieces(const quadboard qb);
bitboard getTeamPieces(const quadboard qb, const byte team);

#endif
