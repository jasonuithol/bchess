//
// Note: 
//
// WHITE == UP   == 0
// BLACK == DOWN == 1
//
#define DIRECTION_UP    (WHITE)
#define DIRECTION_DOWN  (BLACK)

// These are positive-only attack vectors for all pieces except KNIGHT.
#define nw (1ULL << 9)
#define n  (1ULL << 8)
#define ne (1ULL << 7)
#define w  (1ULL << 1)

// These are positive-only attack vectors for KNIGHT.
#define nww (1ULL << 6)
#define nnw (1ULL << 15)
#define nne (1ULL << 17)
#define nee (1ULL << 10)

#define castle (1ULL << 2)

#define queenAttacks    (nw  | n   | ne | w  )
#define bishopAttacks   (nw  | ne            )
#define rookAttacks     (n   | w             )    
#define kingAttacks     (nw  | n   | ne | w  )
#define knightAttacks   (nww | nnw |nne | nee)
#define pawnAttacks     (ne  | nw            )

typedef bitboard (*funcApplyAttack)(const attackContext* const ac, 
                                    const directionalVector* const dc);

typedef struct {
    
    funcApplyAttack applier;

    bitboard positiveVectors;
    bitboard negativeVectors;
    
    byte pieceType;

} vectorsContext;

inline bitboard singlePieceAttacks_Directional(const attackContext* const ac, 
                                        const funcApplyAttack applier,
                                        const bitboard directionalVectors,
                                        const byte direction) {

    bitboard attacks = 0ULL;

    // Create a new vector scratchlist.
    iterator vectors = newIterator(directionalVectors);
    vectors = getNextItem(vectors);
        
    // Iterating over the vectors (if there are any, or any left)
    while (vectors.item) { 
        
        // Apply the vector;
        const directionalVector dc = { vectors.item, direction };
        attacks |= applier(ac, &dc);
        
        // Fetch the next vector (if any left).
        vectors = getNextItem(vectors); 
    } 
    
    return attacks;
}

inline bitboard singlePieceAttacks(const attackContext* const ac, 
                            const vectorsContext* const vc) {

    return
    
        // Apply any positive (UP) vectors.
        singlePieceAttacks_Directional( ac, 
                                        vc->applier, 
                                        vc->positiveVectors, 
                                        DIRECTION_UP    )
        |
        
        // Apply any negative (DOWN) vectors.
        singlePieceAttacks_Directional( ac, 
                                        vc->applier, 
                                        vc->negativeVectors, 
                                        DIRECTION_DOWN  )
        ;
        
}

const vectorsContext pieceVectorContexts[2][7] = {

    //
    // IMPORTANT: Order of elements determines order of execution and therefore
    //            has major impact on pipelining and branch prediction.
    //
    
    // WHITE
    {
        { applySingleAttackVector,  pawnAttacks,    0               , PAWN      },
        { applySingleAttackVector,  kingAttacks,    kingAttacks     , KING      },
        { applySingleAttackVector,  knightAttacks,  knightAttacks   , KNIGHT    },
        { applySlidingAttackVector, rookAttacks,    rookAttacks     , ROOK      },
        { applySlidingAttackVector, bishopAttacks,  bishopAttacks   , BISHOP    },
        { applySlidingAttackVector, queenAttacks,   queenAttacks    , QUEEN     }           
    },
    
    // BLACK
    {
        { applySingleAttackVector,  0,              pawnAttacks     , PAWN      },
        { applySingleAttackVector,  kingAttacks,    kingAttacks     , KING      },
        { applySingleAttackVector,  knightAttacks,  knightAttacks   , KNIGHT    },
        { applySlidingAttackVector, bishopAttacks,  bishopAttacks   , BISHOP    },
        { applySlidingAttackVector, rookAttacks,    rookAttacks     , ROOK      },
        { applySlidingAttackVector, queenAttacks,   queenAttacks    , QUEEN     }
    }
};


inline byte isSquareAttacked(const quadboard qb, const bitboard square, const byte askingTeam) {

    const byte attackingTeam = askingTeam ^ 1;

    const attackContext ac = {
        square,
        getTeamPieces(qb, attackingTeam),
        getTeamPieces(qb, askingTeam)
    };
    
    for (byte i = 0; i < 6; i++) {

        // Choose the vector context for the appropriate piece type (i).
        // IMPORTANT: i does NOT map to pieceType values !
        const vectorsContext* vc = &(pieceVectorContexts[attackingTeam][i]);

        // e.g. get all the black bishops.
        const bitboard enemyPiecesOfAttackType = getPieces(qb, vc->pieceType);

        if (enemyPiecesOfAttackType & singlePieceAttacks(&ac, vc)) {
            // An enemy piece was "attacked" by the square.
            // In other words, the square can be seen by the enemy of that attack type
            // (e.g. a bishop can see the square if the square pretends to be a bishop)
            return 1;
        }
    }

    // Couldn't find any attackers.
    return 0;
}

