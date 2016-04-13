#define BOARD_LEGAL     (1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL    (0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)


typedef struct { // 35 bytes
    quadboard quad;
    byte currentCastlingRights;     // Used to check castling ability for CURRENT move only.
    byte piecesMoved;               // Used to check castling ability for all future moves.
    byte whosTurn;                  // 0 = WHITE, 1 = BLACK.
} board;

typedef int16_t scoreType;

typedef struct {

    bitboard from;
    bitboard to;
    scoreType score;
    byte promoteTo;
    board resultingBoard;
    
} analysisMove;


#define ANALYSIS_SIZE (255)

typedef struct {
    
    analysisMove items[ANALYSIS_SIZE];
    byte ix;
    
} analysisList; 


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

    // Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
    moveSquare(qb, from, to);

    // Change the turn on the new board.  To get the team that just moved, use old->whosTurn.
    new->whosTurn = old->whosTurn ^ 1;


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
    if (getPieces(old->quad, KING | old->whosTurn) & from) {
        if (getFile(from) - getFile(to) == 2) {
            // Kingside castle detected
            moveSquare(qb, 1ULL, 4ULL);
        }
        else if (getFile(from) - getFile(to) == -2) {
            // Queenside castle detected
            moveSquare(qb, 128ULL, 16ULL);
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
        case toBitboard('d','1'): new->piecesMoved |= WHITE_KING_MOVED;             break;
        case toBitboard('h','1'): new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED;  break;
        case toBitboard('a','8'): new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
        case toBitboard('d','8'): new->piecesMoved |= BLACK_KING_MOVED;             break;
        case toBitboard('h','8'): new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED;  break;
    }

    const bitboard enemies = getTeamPieces(new->quad, old->whosTurn);
    const bitboard friends = getTeamPieces(new->quad, new->whosTurn);

    //
    // We need this for checking to see if in the next move, a castling move is allowed
    //
    // NOTE: FOR LEAF BOARDS, THIS IS A NEEDLESS COST !!!!!
    //
    new->currentCastlingRights = new->piecesMoved;
    
    const char rank = new->whosTurn ? '8' : '1';

    //
    // KINGSIDE
    //
                                            
    // Check if squares are occupied.
    if ( (enemies|friends) & (toBitboard('f',rank) | toBitboard('g',rank) ) ) {

        new->currentCastlingRights |= new->whosTurn 
                                        ? BLACK_KINGSIDE_CASTLE_MOVED 
                                        : WHITE_KINGSIDE_CASTLE_MOVED;
    }
    else {
        
        // Check if squares are attacked
        for (char file = 'e'; file < 'h'; file++) {
            
            if (isSquareAttacked(new->quad, toBitboard(file,rank), new->whosTurn)) {
                
                new->currentCastlingRights |= new->whosTurn 
                                                ? BLACK_KINGSIDE_CASTLE_MOVED 
                                                : WHITE_KINGSIDE_CASTLE_MOVED;
                break;
            }
        }
        
    }

    //
    // QUEENSIDE
    //
                                            
    // Check if squares are occupied.
    if ( (enemies|friends) & (toBitboard('b',rank) | toBitboard('c',rank) | toBitboard('d',rank) ) ) {

        new->currentCastlingRights |= new->whosTurn 
                                        ? BLACK_QUEENSIDE_CASTLE_MOVED 
                                        : WHITE_QUEENSIDE_CASTLE_MOVED;
    }
    else {
        
        // Check if squares are attacked
        for (char file = 'a'; file < 'e'; file++) {
            
            if (isSquareAttacked(new->quad, toBitboard(file,rank), new->whosTurn)) {
                
                new->currentCastlingRights |= new->whosTurn 
                                                ? BLACK_QUEENSIDE_CASTLE_MOVED 
                                                : WHITE_QUEENSIDE_CASTLE_MOVED;
                break;
            }
        }
        
    }

    // Board passed the illegal check state test earlier, so board is legal.
    return BOARD_LEGAL;

}

                    

byte addMoveIfLegal(    analysisList* const list, 
                        const board* const old, 
                        const bitboard from, 
                        const bitboard to, 
                        const byte promoteTo, 
                        const byte leafMode) {

    if (list->ix < ANALYSIS_SIZE) {
        
        analysisMove* const next = &(list->items[list->ix]);
        
        next->from      = from;
        next->to        = to;
        next->score     = 0;
        next->promoteTo = promoteTo;       
        
        const byte legality = leafMode 
                            ? spawnLeafBoard(old, &(next->resultingBoard), from, to, promoteTo)
                            : spawnFullBoard(old, &(next->resultingBoard), from, to, promoteTo);

        if (legality == BOARD_LEGAL) {
            // Keep this board.
            list->ix++;
        }
        
        //
        // Tell caller what happened re legality 
        // (illegal implies that move was thrown out)
        //
        return legality;
    }
    else {
        error("\nMaximum analysis moves size exceeded.\n");
    }

}

void generateLegalMoveList(const board* const b, analysisList* const moveList, const byte leafMode) {
    
    const bitboard friends = getTeamPieces(b->quad, b->whosTurn);
    const bitboard enemies = getTeamPieces(b->quad, b->whosTurn ^ 1);

    // Excludes King
    for (byte pieceType = PAWN; pieceType < KING; pieceType += 2) {

        iterator piece = newIterator(getPieces(b->quad, pieceType | b->whosTurn));
        piece = getNextItem(piece);
            
        while (piece.item) {
            
            bitboard moves = 0ULL;
            
            switch(pieceType) {
                case PAWN:   moves = generatePawnMoves(piece.item, enemies, friends, b->whosTurn);    break;
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

void printAllowedMoves(const board* const b) {
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
byte detectCheckmate(const board* const b) {
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
    b->currentCastlingRights = WHITE_KINGSIDE_CASTLE_MOVED
                                | WHITE_QUEENSIDE_CASTLE_MOVED
                                | BLACK_KINGSIDE_CASTLE_MOVED
                                | BLACK_QUEENSIDE_CASTLE_MOVED;
    b->whosTurn = WHITE;
}

