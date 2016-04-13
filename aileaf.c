
#define SCORE_PAWN          (1)
#define SCORE_KNIGHT        (4)
#define SCORE_BISHOP        (6)
#define SCORE_ROOK          (14)
#define SCORE_QUEEN         (21)
//#define SCORE_KING_CHECK  (10)

typedef uint32_t nodesCalculatedType;
typedef uint8_t depthType;

//
// Animates a little spinner to show that we are still alive.
//
// IMPORTANT: Do not use print() !! Instead, use printf() 
// We don't want this going into the logs !!!!
//

nodesCalculatedType nodesCalculated;

#define PULSE_SPIN_MAGNITUDE (17)

void displaySpinningPulse() {

    const nodesCalculatedType wrapMask = (1 << (PULSE_SPIN_MAGNITUDE)) - 1;

    nodesCalculated++; 

    switch (nodesCalculated & wrapMask) {
        case 0:
            printf("/\b");
            fflush(stdout);
            break;
        case (1 << (PULSE_SPIN_MAGNITUDE - 2)):
            printf("-\b");
            fflush(stdout);
            break;
        case (1 << (PULSE_SPIN_MAGNITUDE - 1)):
            printf("\\\b");
            fflush(stdout);
            break;
        case (1 << (PULSE_SPIN_MAGNITUDE - 2)) | (1 << (PULSE_SPIN_MAGNITUDE - 1)):
            printf("|\b");
            fflush(stdout);
            break;
    }
}


//
// MATERIAL: A cheap and cheerful strategy for deciding the best move
//           based on how much material the player has at the end of 
//           each possible move.
//

inline scoreType evaluateMaterial(const quadboard qb, const byte team) {

    return    SCORE_PAWN    * populationCount(getPieces(qb, PAWN   | team))
            + SCORE_KNIGHT  * populationCount(getPieces(qb, KNIGHT | team))
            + SCORE_BISHOP  * populationCount(getPieces(qb, BISHOP | team))
            + SCORE_ROOK    * populationCount(getPieces(qb, ROOK   | team))
            + SCORE_QUEEN   * populationCount(getPieces(qb, QUEEN  | team))
            
            - SCORE_PAWN    * populationCount(getPieces(qb, PAWN   | (team ^ 1)))
            - SCORE_KNIGHT  * populationCount(getPieces(qb, KNIGHT | (team ^ 1)))
            - SCORE_BISHOP  * populationCount(getPieces(qb, BISHOP | (team ^ 1)))
            - SCORE_ROOK    * populationCount(getPieces(qb, ROOK   | (team ^ 1)))
            - SCORE_QUEEN   * populationCount(getPieces(qb, QUEEN  | (team ^ 1))) ;
            
}


//
// MOBILITY: A mildly expensive strategy for deciding the best move 
//           based on how many more possible PSUEDOLEGAL moves the player
//           has than it's opponent.
//

typedef bitboard (getterFuncPtr)(const quadboard, const byte); 
typedef bitboard (generatorFuncPtr)(const bitboard, const bitboard, const bitboard); 

inline scoreType countMoves(    const quadboard qb, 
                        generatorFuncPtr generator, 
                        const bitboard friends, 
                        const bitboard enemies, 
                        const byte pieceType) {
    
    scoreType subscore = 0;
    
    iterator piece = newIterator(getPieces(qb, pieceType)); 
    piece = getNextItem(piece);
    
    while (piece.item) {    
        
//      print("\ncountMoves counting new move\n");
//      printBB(generator(piece.item, friends, enemies));
        
        // Add the number of moves this piece can make to the tally.
        //
        // NOTE: The call to generator probably blows our cache 
        //       right out of the water.
        subscore += (scoreType)populationCount(generator(piece.item, enemies, friends));
        
        piece = getNextItem(piece);
    }       
    
    return subscore;
}

inline scoreType evaluateMobility(const quadboard qb, const byte team) {
    
    const bitboard friends = getTeamPieces(qb, team);
    const bitboard enemies = getTeamPieces(qb, team ^ 1);
            
    return    countMoves(qb, generateKnightMoves, friends, enemies, KNIGHT | team)
            + countMoves(qb, generateBishopMoves, friends, enemies, BISHOP | team)
            + countMoves(qb, generateRookMoves,   friends, enemies, ROOK   | team)
            + countMoves(qb, generateQueenMoves,  friends, enemies, QUEEN  | team)
            // For the very moment, skipping kings and pawns. */

            
            - countMoves(qb, generateKnightMoves, enemies, friends, KNIGHT | (team ^ 1))
            - countMoves(qb, generateBishopMoves, enemies, friends, BISHOP | (team ^ 1))
            - countMoves(qb, generateRookMoves,   enemies, friends, ROOK   | (team ^ 1))
            - countMoves(qb, generateQueenMoves,  enemies, friends, QUEEN  | (team ^ 1))
            ;
            // For the very moment, skipping kings and pawns.

}

inline scoreType analyseLeafNonTerminal(const quadboard qb, const byte team) {
    
    // Tell the world we still live.
    displaySpinningPulse();
        
    // We have hit the limit of our depth search - time to score the board.
    return (1 * evaluateMobility(qb, team))
         + (4 * evaluateMaterial(qb, team));    

}

scoreType analyseLeafTerminal(const board* const b, const byte scoringTeam, const depthType depth) {

    const byte boardState = determineEndOfGameState(b);

    if (b->whosTurn == scoringTeam) {
        
        if (boardState == BOARD_CHECKMATE) {
            return -9998 + (depth);
        }
        else {
            return 0;
        }
    }
    else {
                
        if (boardState == BOARD_CHECKMATE) {
            return 9998 - (depth);
        }
        else {
            return 0;
        }
    }
    
}


