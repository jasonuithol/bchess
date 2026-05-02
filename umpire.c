#include <string.h>

#include "umpire.h"
#include "attacks.h"
#include "bitboard.h"
#include "iterator.h"
#include "logging.h"
#include "quadboard.h"
#include "tt.h"  // zobPiece / zobSide / zobCastle / zobEnPassant for incremental hash


void clearBoard(board* const b) {
    memset((void*)b, 0, sizeof(board));
}

// After one generates pawn moves, one must call this method to see if extra information
// needs to be passed to spawnXXXBoard()
byte isPawnPromotable(const bitboard piece) {
    return (piece < (1ULL << 8) || piece > (1ULL << 55));
}

byte isKingChecked(const quadboard qb, byte team) {
    return isSquareAttacked(qb, getPieces(qb, KING | team), team);
}

//
// PRECONDITION: Only call this if there were no legal moves to make.
//
byte determineEndOfGameState(const board* const b) {
    return isKingChecked(b->quad, b->whosTurn)
           ? BOARD_CHECKMATE
           : BOARD_STALEMATE;
}


//
// All moves MUST be performed by this method to ensure that:
//
// * Castling is managed properly.
// * Pawns get promoted.
// * Illegal board positions (involving check) are thrown out.
//
byte spawnLeafBoard(const board* const old,
                    board* const new,
                    const bitboard from,
                    const bitboard to,
                    const byte promoteTo) {


    quadboard* const qb = &(new->quad);

    // To create a new board from a current board, first copy the current board's content to the new one.
    memcpy((void*)new, (void*)old, sizeof(board));

    // En-passant detection: a pawn moving onto old->enPassantTarget by
    // a diagonal step is an e.p. capture, because that square is empty
    // (no enemy on it) and yet the pawn is moving diagonally — the only
    // way that can be legal is e.p. The captured pawn doesn't sit on
    // the destination square; it sits on the same file as the
    // destination but the same rank as the source. We have to clear it
    // explicitly because moveSquare won't touch that square.
    const byte movingIsPawn = (getPieces(old->quad, PAWN | old->whosTurn) & from) != 0;
    if (movingIsPawn
        && old->enPassantTarget != 0
        && to == old->enPassantTarget
        && getFile(from) != getFile(to)) {
        const offset capturedFile = getFile(to);
        const offset capturedRank = getRank(from);
        const bitboard capturedSq = 1ULL << (capturedRank * 8 + capturedFile);
        resetSquares(qb, capturedSq);
    }

    // Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
    moveSquare(qb, from, to);

    // Change the turn on the new board.  To get the team that just moved, use old->whosTurn.
    new->whosTurn = old->whosTurn ^ 1;

    // Default: clear any e.p. opportunity. We re-set it below if this
    // very move was a pawn double-push.
    new->enPassantTarget = 0;
    if (movingIsPawn) {
        const offset fromRank = getRank(from);
        const offset toRank   = getRank(to);
        const int rankDelta = (int)toRank - (int)fromRank;
        if (rankDelta == 2 || rankDelta == -2) {
            // The square the pawn skipped over — the landing pad for an
            // enemy e.p. capture on the next ply.
            const offset skippedRank = (fromRank + toRank) / 2;
            new->enPassantTarget = 1ULL << (skippedRank * 8 + getFile(to));
        }
    }


    // Now we can perform the legality check
    //
    if (isSquareAttacked(new->quad, getPieces(new->quad, KING | old->whosTurn), old->whosTurn)) {
        return BOARD_NOT_LEGAL;
    }

    
    //
    // Pawn promotion followup (takes the chosen promotion and applies it)
    //
    if (promoteTo > 0) {
        resetSquares(qb, to);
        addPieces(qb, to, promoteTo | old->whosTurn);
    }

    //
    // Castling followup (moves the castle over the king).
    //
    // Bit-layout reminder: within a rank the h-file is bit 0 and the
    // a-file is bit 7, so the rook squares we want to act on are at
    // (rank*8 + 0) for the h-rook and (rank*8 + 7) for the a-rook.
    // The original code hardcoded rank 1, which meant every black
    // castle silently overwrote whatever sat on d1/f1 with whatever
    // sat on a1/h1 (e.g. a black O-O-O would clobber white's queen
    // with white's a1 rook).
    //
    if (getPieces(old->quad, KING | old->whosTurn) & from) {
        const offset rankShift = getRank(from) * 8;
        if (getFile(from) - getFile(to) == 2) {
            // Kingside castle: h-rook -> f-rook
            moveSquare(qb, 1ULL << rankShift, 1ULL << (rankShift + 2));
        }
        else if (getFile(from) - getFile(to) == -2) {
            // Queenside castle: a-rook -> d-rook
            moveSquare(qb, 1ULL << (rankShift + 7), 1ULL << (rankShift + 4));
        }
    }


    // Board passed the illegal check state test earlier, so board is legal.
    return BOARD_LEGAL;
}


//
// If an AI is pondering a move higher than leaf level, use this method.
//
// Since this board is going to have future moves made against it, maintain a bit more
// state.  It's more expensive, but required for boards that actually get played on.
//
// The vast, vast majority of leaf boards never get spawnXXXBoard called on them,
// only the Chosen Ones do. Use spawnLeafBoard for leaf boards.
//
//
// Compute the castling-rights overlay for the team about to move (b->whosTurn).
//
// piecesMoved is the persistent "this rook/king has moved" bitset. On top of
// that we layer transient reasons castling is unavailable right now: pieces
// in the way between king and rook, or any square in the king's path being
// attacked. The result is what generateKingMoves consults to decide whether
// to emit O-O / O-O-O for the current player.
//
void computeCurrentCastlingRights(board* const b) {

    const byte team = b->whosTurn;
    const bitboard occupied = getAllPieces(b->quad);
    const char rank = team ? '8' : '1';

    b->currentCastlingRights = b->piecesMoved;

    const byte kingsideMask  = team ? BLACK_KINGSIDE_CASTLE_MOVED  : WHITE_KINGSIDE_CASTLE_MOVED;
    const byte queensideMask = team ? BLACK_QUEENSIDE_CASTLE_MOVED : WHITE_QUEENSIDE_CASTLE_MOVED;

    //
    // KINGSIDE: f and g must be empty; e, f, g must be unattacked
    // (king cannot castle out of, through, or into check).
    //
    if (occupied & (toBitboard('f',rank) | toBitboard('g',rank))) {
        b->currentCastlingRights |= kingsideMask;
    }
    else {
        for (char file = 'e'; file <= 'g'; file++) {
            if (isSquareAttacked(b->quad, toBitboard(file,rank), team)) {
                b->currentCastlingRights |= kingsideMask;
                break;
            }
        }
    }

    //
    // QUEENSIDE: b, c, d must be empty; c, d, e must be unattacked.
    // (b is on the rook's path but not the king's, so it's an occupancy
    // check only.)
    //
    if (occupied & (toBitboard('b',rank) | toBitboard('c',rank) | toBitboard('d',rank))) {
        b->currentCastlingRights |= queensideMask;
    }
    else {
        for (char file = 'c'; file <= 'e'; file++) {
            if (isSquareAttacked(b->quad, toBitboard(file,rank), team)) {
                b->currentCastlingRights |= queensideMask;
                break;
            }
        }
    }
}


byte spawnFullBoard(const board* const old,
                    board* const new,
                    const bitboard from,
                    const bitboard to,
                    const byte promoteTo) {


    // First, spawn a leaf board, it will have all the tidying up and illegal position checks.
    if (spawnLeafBoard(old, new, from, to, promoteTo) == BOARD_NOT_LEGAL) {

        // Board was found to be illegal, abort and notify caller.
        return BOARD_NOT_LEGAL;
    }

    //
    // To see if it's still possible to castle in the future,
    // we track whether the relevant pieces have moved.
    //
    switch(from) {
        case toBitboard('a','1'): new->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('e','1'): new->piecesMoved |= WHITE_KING_MOVED;             break;
        case toBitboard('h','1'): new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED;  break;
        case toBitboard('a','8'): new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('e','8'): new->piecesMoved |= BLACK_KING_MOVED;             break;
        case toBitboard('h','8'): new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED;  break;
    }

    // A rook can also leave a corner by being captured there. The
    // from-square doesn't see this — the corner stays the to-square of
    // the capturing piece — so we have to mark it explicitly. Without
    // this, a side whose corner rook has been taken still thinks it
    // can castle, and bchess generates illegal castling moves.
    switch(to) {
        case toBitboard('a','1'): new->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('h','1'): new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED;  break;
        case toBitboard('a','8'): new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('h','8'): new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED;  break;
    }

    // Compute castling rights for the team that is now to move.
    computeCurrentCastlingRights(new);

    // Cold path: recompute hash from scratch. spawnFullBoard / spawnLeafBoard
    // are only used for moves actually played (uci position application,
    // human-vs-engine, root-played move), not for search nodes — search
    // goes through applyMove/revertMove which keeps hash incrementally. A
    // one-shot 64-square scan per real move is fine.
    new->hash = computeZobristHash(new);

    // Board passed the illegal check state test earlier, so board is legal.
    return BOARD_LEGAL;

}

                    

// In-place make. Mutates b; writes undo info to *undo. Together with
// revertMove, replaces spawnFullBoard for the hot search/legality paths.
//
// Order matters: detect what kind of move this is from the OLD state
// before any mutation, then apply piece movement, promotion, castle-rook
// follow-up, piecesMoved updates, ep target, side flip. b->hash is
// XOR-updated incrementally at each step; revertMove restores it from
// undo->prevHash directly.
void applyMove(board* const b, const analysisMove* const move, UndoInfo* const undo, byte updateHash) {

    quadboard* const qb = &(b->quad);
    const byte mover    = b->whosTurn;
    const bitboard from = move->from;
    const bitboard to   = move->to;
    const byte promoteTo = move->promoteTo;
    const offset fromOffset = trailingBit_Bitboard(from);
    const offset toOffset   = trailingBit_Bitboard(to);

    // Pre-mutation classification. Reads the old state.
    const byte movingIsPawn = (getPieces(*qb, PAWN | mover) & from) != 0;
    const byte movingIsKing = (getPieces(*qb, KING | mover) & from) != 0;
    const int  fileDelta    = (int)getFile(from) - (int)getFile(to);
    const byte movingPiece  = updateHash ? (getType(*qb, fromOffset) | mover) : 0;

    // Save everything we're about to clobber.
    undo->prevEnPassantTarget = b->enPassantTarget;
    undo->prevCastlingRights  = b->currentCastlingRights;
    undo->prevPiecesMoved     = b->piecesMoved;
    undo->prevHash            = b->hash;
    undo->capturedSquare      = 0;
    undo->capturedPiece       = 0;
    undo->movedTeam           = mover;

    // Hash: pre-emptively XOR out the OLD castle and ep keys; we'll XOR
    // the NEW ones in at the bottom once piecesMoved and enPassantTarget
    // have settled. Keeping these at the boundary avoids an inner XOR
    // at every piecesMoved-modifying switch case.
    if (updateHash) {
        b->hash ^= zobCastle[b->piecesMoved];
        if (b->enPassantTarget) {
            const offset epFile = trailingBit_Bitboard(b->enPassantTarget) % 8;
            b->hash ^= zobEnPassant[epFile];
        } else {
            b->hash ^= zobEnPassant[8];
        }
    }

    // EP capture: pawn moves diagonally onto the ep target. The captured
    // pawn is on the file of `to` and the rank of `from`, not on `to`
    // itself — moveSquare won't touch it, so we have to clear it (and
    // remember it for unmake).
    if (movingIsPawn
        && b->enPassantTarget != 0
        && to == b->enPassantTarget
        && getFile(from) != getFile(to)) {
        const offset capSqOffset = getRank(from) * 8 + getFile(to);
        const bitboard capturedSq = 1ULL << capSqOffset;
        const byte    capPiece    = getType(*qb, capSqOffset) | getTeam(*qb, capSqOffset);
        undo->capturedSquare = capturedSq;
        undo->capturedPiece  = capPiece;
        if (updateHash) b->hash ^= zobPiece[capSqOffset][capPiece];
        resetSquares(qb, capturedSq);
    }
    // Normal capture: there's an enemy piece sitting on `to`. moveSquare
    // will wipe it as a side-effect, so we have to save its identity now.
    else if (getAllPieces(*qb) & to) {
        const byte capPiece = getType(*qb, toOffset) | getTeam(*qb, toOffset);
        undo->capturedSquare = to;
        undo->capturedPiece  = capPiece;
        if (updateHash) b->hash ^= zobPiece[toOffset][capPiece];
    }

    // Apply the piece movement.
    if (updateHash) {
        b->hash ^= zobPiece[fromOffset][movingPiece];
        b->hash ^= zobPiece[toOffset]  [movingPiece];
    }
    moveSquare(qb, from, to);

    // Promotion: replace the just-moved pawn with the chosen piece.
    if (promoteTo > 0) {
        if (updateHash) {
            b->hash ^= zobPiece[toOffset][movingPiece];                // remove pawn
            b->hash ^= zobPiece[toOffset][promoteTo | mover];          // add promoted
        }
        resetSquares(qb, to);
        addPieces(qb, to, promoteTo | mover);
    }

    // Castling follow-up: if the king moved exactly two files, slide the
    // matching rook to its post-castle square. Bit-layout is h=0..a=7
    // within a rank, so kingside = file delta +2 (e->g), queenside = -2.
    if (movingIsKing) {
        const offset rankShift = getRank(from) * 8;
        if (fileDelta == 2) {
            const offset rFrom = rankShift;
            const offset rTo   = rankShift + 2;
            if (updateHash) {
                b->hash ^= zobPiece[rFrom][ROOK | mover];
                b->hash ^= zobPiece[rTo]  [ROOK | mover];
            }
            moveSquare(qb, 1ULL << rFrom, 1ULL << rTo);
        }
        else if (fileDelta == -2) {
            const offset rFrom = rankShift + 7;
            const offset rTo   = rankShift + 4;
            if (updateHash) {
                b->hash ^= zobPiece[rFrom][ROOK | mover];
                b->hash ^= zobPiece[rTo]  [ROOK | mover];
            }
            moveSquare(qb, 1ULL << rFrom, 1ULL << rTo);
        }
    }

    // Castling-rights bookkeeping (persistent has-moved flags).
    switch(from) {
        case toBitboard('a','1'): b->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('e','1'): b->piecesMoved |= WHITE_KING_MOVED;             break;
        case toBitboard('h','1'): b->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED;  break;
        case toBitboard('a','8'): b->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('e','8'): b->piecesMoved |= BLACK_KING_MOVED;             break;
        case toBitboard('h','8'): b->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED;  break;
    }
    // Corner-rook captured: same flag, triggered by the to-square.
    switch(to) {
        case toBitboard('a','1'): b->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('h','1'): b->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED;  break;
        case toBitboard('a','8'): b->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('h','8'): b->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED;  break;
    }

    // EP target: set only if THIS move was a pawn double-push.
    b->enPassantTarget = 0;
    if (movingIsPawn) {
        const offset fromRank = getRank(from);
        const offset toRank   = getRank(to);
        const int rankDelta = (int)toRank - (int)fromRank;
        if (rankDelta == 2 || rankDelta == -2) {
            const offset skippedRank = (fromRank + toRank) / 2;
            b->enPassantTarget = 1ULL << (skippedRank * 8 + getFile(to));
        }
    }

    // Hash: XOR in NEW castle + ep keys, and toggle side.
    if (updateHash) {
        b->hash ^= zobCastle[b->piecesMoved];
        if (b->enPassantTarget) {
            const offset epFile = trailingBit_Bitboard(b->enPassantTarget) % 8;
            b->hash ^= zobEnPassant[epFile];
        } else {
            b->hash ^= zobEnPassant[8];
        }
        b->hash ^= zobSide;
    }

    b->whosTurn = mover ^ 1;

    // Note: we deliberately DON'T call computeCurrentCastlingRights
    // here. It's only needed when the next move-generation pass will
    // consult the rights — i.e. when the search is about to recurse.
    // The legality probe in addMoveIfLegal and leaf-depth applyMove
    // calls discard the post-move castling rights, so paying for them
    // at every node would be pure waste (and was the dominant cost
    // when applyMove computed them unconditionally). Search code that
    // recurses calls computeCurrentCastlingRights explicitly.
}


// In-place unmake. Reverses applyMove using only undo + the move itself.
// Symmetric pairing is a hard correctness requirement — perft will scream
// if any state isn't perfectly restored.
void revertMove(board* const b, const analysisMove* const move, const UndoInfo* const undo) {

    quadboard* const qb = &(b->quad);
    const byte mover    = undo->movedTeam;
    const bitboard from = move->from;
    const bitboard to   = move->to;
    const byte promoteTo = move->promoteTo;

    // After applyMove, the king is on `to` — that's how we re-detect a
    // king move. Promotion can't yield a king, so this is unambiguous.
    const byte movingIsKing = (getPieces(*qb, KING | mover) & to) != 0;
    const int  fileDelta    = (int)getFile(from) - (int)getFile(to);

    // Undo the castling rook BEFORE we move the king back, so the rook
    // is on its original h/a square first and the subsequent moveSquare
    // for the king doesn't trip over it.
    if (movingIsKing) {
        const offset rankShift = getRank(from) * 8;
        if (fileDelta == 2) {
            moveSquare(qb, 1ULL << (rankShift + 2), 1ULL << rankShift);
        }
        else if (fileDelta == -2) {
            moveSquare(qb, 1ULL << (rankShift + 4), 1ULL << (rankShift + 7));
        }
    }

    // Undo promotion: turn the promoted piece back into a pawn before
    // we move it home.
    if (promoteTo > 0) {
        resetSquares(qb, to);
        addPieces(qb, to, PAWN | mover);
    }

    // Move the piece back. moveSquare clears `from` first then writes
    // `to`'s contents to `from`, so this works even if the captured
    // piece's bits are still in the picture (they're not — see below).
    moveSquare(qb, to, from);

    // Restore captured piece. moveSquare(to, from) wiped `to` clean as
    // part of moving the piece off it, so addPieces is safe (precondition
    // of addPieces is that target squares are 0000).
    if (undo->capturedPiece) {
        addPieces(qb, undo->capturedSquare, undo->capturedPiece);
    }

    b->enPassantTarget       = undo->prevEnPassantTarget;
    b->piecesMoved           = undo->prevPiecesMoved;
    b->currentCastlingRights = undo->prevCastlingRights;
    b->whosTurn              = mover;
    b->hash                  = undo->prevHash;
}


byte addMoveIfLegal(    analysisList* const list,
                        board* const old,
                        const bitboard from,
                        const bitboard to,
                        const byte promoteTo,
                        const byte leafMode) {
    (void)leafMode;  // legality is now state-independent of leaf vs full

    if (list->ix < ANALYSIS_SIZE) {

        // Make/unmake to test legality: the move is illegal iff making
        // it leaves the mover's king in check. No board copy required.
        const analysisMove probe = { .from = from, .to = to, .score = 0, .promoteTo = promoteTo };
        UndoInfo undo;
        // updateHash=0: legality only reads quad and isSquareAttacked,
        // not b->hash. Skipping the XOR work is the dominant per-move
        // saving from incremental hashing.
        applyMove(old, &probe, &undo, 0);
        const byte legal = !isSquareAttacked(old->quad, getPieces(old->quad, KING | undo.movedTeam), undo.movedTeam);
        revertMove(old, &probe, &undo);

        if (legal) {
            analysisMove* const next = &(list->items[list->ix]);
            next->from      = from;
            next->to        = to;
            next->score     = 0;
            next->promoteTo = promoteTo;
            list->ix++;
            return BOARD_LEGAL;
        }
        return BOARD_NOT_LEGAL;
    }
    else {
        error("\nMaximum analysis moves size exceeded.\n");
        return BOARD_NOT_LEGAL;
    }

}

void generateLegalMoveList(board* const b, analysisList* const moveList, const byte leafMode) {
    
    const bitboard friends = getTeamPieces(b->quad, b->whosTurn);
    const bitboard enemies = getTeamPieces(b->quad, b->whosTurn ^ 1);

    // Excludes King
    for (byte pieceType = PAWN; pieceType < KING; pieceType += 2) {

        iterator piece = newIterator(getPieces(b->quad, pieceType | b->whosTurn));
        piece = getNextItem(piece);
            
        while (piece.item) {
            
            bitboard moves = 0ULL;
            
            switch(pieceType) {
                case PAWN:   moves = generatePawnMoves(piece.item, enemies, friends, b->whosTurn, b->enPassantTarget);    break;
                case ROOK:   moves = generateRookMoves(piece.item, enemies, friends);    break;
                case KNIGHT: moves = generateKnightMoves(piece.item, enemies, friends);  break;
                case BISHOP: moves = generateBishopMoves(piece.item, enemies, friends);  break;
                case QUEEN:  moves = generateQueenMoves(piece.item, enemies, friends);   break;
                default: error("Bad pieceType\n");
            }
                        
            iterator move = newIterator(moves);
            move = getNextItem(move);

            while (move.item) {
                
                if (pieceType == PAWN && isPawnPromotable(move.item)) {
                    addMoveIfLegal(moveList, b, piece.item, move.item, QUEEN, leafMode);
                    addMoveIfLegal(moveList, b, piece.item, move.item, KNIGHT, leafMode);
                    addMoveIfLegal(moveList, b, piece.item, move.item, BISHOP, leafMode);
                    addMoveIfLegal(moveList, b, piece.item, move.item, ROOK, leafMode);
                }
                else {
                    addMoveIfLegal(moveList, b, piece.item, move.item, 0, leafMode);
                }
                
                move = getNextItem(move);
            }
            
            piece = getNextItem(piece);
        }

    }

    //
    // King is a special case
    //
    {
        const bitboard king = getPieces(b->quad, KING | b->whosTurn);
        const bitboard moves = generateKingMoves(king, enemies, friends, b->currentCastlingRights, b->whosTurn);

        iterator move = newIterator(moves);
        move = getNextItem(move);

        while (move.item) {
            addMoveIfLegal(moveList, b, king, move.item, 0, leafMode);
            move = getNextItem(move);
        }
    }

}



// ==========================================================
//
//                      WRAPPER FUNCTIONS
//
// ==========================================================



void printMove(const analysisMove move) {
    print("[");
    printSquare(move.from);
    print(",");
    printSquare(move.to);
    print("]");
}

void printMoveList(const analysisList* const moveList) {
    for (int i = 0; i < moveList->ix; i++) {
        printMove(moveList->items[i]);
        print("\n");
    }
}

void printAllowedMoves(board* const b) {
    analysisList moveList;
    moveList.ix = 0;
    generateLegalMoveList(b, &moveList, 0);
    printMoveList(&moveList);
}

//
// When an AI or human chooses a real move to play, use this method.
// Wrapper function simply to hide implementation details of analysisMove.
//
byte makeMove(const board* const old, board* const new, const analysisMove* const move) {
    return spawnFullBoard(old,new,move->from,move->to,move->promoteTo);
}

//
// After making a real move, call this to see if the game has ended
//
byte detectCheckmate(board* const b) {
    analysisList moveList;
    moveList.ix = 0;
    generateLegalMoveList(b,&moveList,0);
    if (moveList.ix == 0) {
        return determineEndOfGameState(b);
    }
    else {
        return BOARD_NORMAL;
    }
}

//
// Prepare the board for a standard game.
//
void initBoard(board* const b) {
    
    quadboard* const qb = &(b->quad);
    
    clearBoard(b);
    
    addPieces(qb, 255ULL << (8 * 1), PAWN | WHITE);
    addPieces(qb, 255ULL << (8 * 6), PAWN | BLACK);

    addPieces(qb, (128ULL + 1), ROOK | WHITE);
    addPieces(qb, (128ULL + 1) << (8 * 7), ROOK | BLACK);

    addPieces(qb, (64ULL + 2), KNIGHT | WHITE);
    addPieces(qb, (64ULL + 2) << (8 * 7), KNIGHT | BLACK);

    addPieces(qb, (32ULL + 4), BISHOP | WHITE);
    addPieces(qb, (32ULL + 4) << (8 * 7), BISHOP | BLACK);

    addPieces(qb, 16ULL, QUEEN | WHITE);
    addPieces(qb, 16ULL << (8 * 7), QUEEN | BLACK);

    addPieces(qb, 8ULL, KING | WHITE);
    addPieces(qb, 8ULL << (8 * 7), KING | BLACK);

    b->piecesMoved = 0;
    b->whosTurn = WHITE;
    b->enPassantTarget = 0;

    // Compute the castling-rights overlay for the first move so that
    // generateKingMoves doesn't offer castling through occupied squares.
    computeCurrentCastlingRights(b);

    // Cold-path one-shot: hash starts in sync with the rest of the board.
    // applyMove takes over from here with incremental XOR updates.
    b->hash = computeZobristHash(b);
}

