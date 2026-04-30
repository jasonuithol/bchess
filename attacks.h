#ifndef ATTACKS_H
#define ATTACKS_H

#include "bitboard.h"
#include "quadboard.h"

#define WHITE_QUEENSIDE_CASTLE_MOVED    (128)
#define WHITE_KING_MOVED                (64)
#define WHITE_KINGSIDE_CASTLE_MOVED     (32)
#define BLACK_QUEENSIDE_CASTLE_MOVED    (16)
#define BLACK_KING_MOVED                (8)
#define BLACK_KINGSIDE_CASTLE_MOVED     (4)

#define ATTACKMODE_SINGLE   (0)
#define ATTACKMODE_SLIDING  (1)
#define ATTACKMODE_PAWN     (2)

#define DIRECTION_UP    (WHITE)
#define DIRECTION_DOWN  (BLACK)

bitboard applySlidingAttackVector(  const bitboard piece,
                                    const bitboard vector,
                                    const bitboard softBlockers,
                                    const bitboard hardBlockers,
                                    const byte direction);

byte isSquareAttacked(const quadboard qb, const bitboard square, const byte askingTeam);

bitboard generateQueenMoves(const bitboard piece, const bitboard enemies, const bitboard friends);
bitboard generateBishopMoves(const bitboard piece, const bitboard enemies, const bitboard friends);
bitboard generateRookMoves(const bitboard piece, const bitboard enemies, const bitboard friends);
bitboard generateKnightMoves(const bitboard piece, const bitboard enemies, const bitboard friends);
bitboard generateKingMoves(const bitboard piece, const bitboard enemies, const bitboard friends, const bitboard currentCastlingRights, const byte team);
bitboard generatePawnMoves(const bitboard piece, const bitboard enemies, const bitboard friends, const byte team);

#endif
