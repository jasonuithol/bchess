void testDisappearingKing() {

    //
    // BOARD SETUP
    //
    board b;
    quadboard* qb = &(b.quad);
    clearBoard(&b);

    b.piecesMoved = ~1;
    b.currentCastlingRights = ~1;
    b.whosTurn = WHITE;
    
    addPieces(qb, toBitboard('a','8'), BLACK | ROOK);
    addPieces(qb, toBitboard('h','8'), BLACK | KING);

    addPieces(qb, toBitboard('a','7'), BLACK | PAWN);
    addPieces(qb, toBitboard('b','7'), BLACK | PAWN);
    addPieces(qb, toBitboard('g','7'), BLACK | PAWN);
    addPieces(qb, toBitboard('h','7'), BLACK | PAWN);
    
    addPieces(qb, toBitboard('c','6'), BLACK | KNIGHT);

    addPieces(qb, toBitboard('d','5'), BLACK | KNIGHT);
    addPieces(qb, toBitboard('f','5'), BLACK | ROOK);

    addPieces(qb, toBitboard('a','4'), WHITE | PAWN);
    addPieces(qb, toBitboard('b','4'), WHITE | PAWN);
    addPieces(qb, toBitboard('f','4'), BLACK | PAWN);
    addPieces(qb, toBitboard('h','4'), WHITE | BISHOP);

    addPieces(qb, toBitboard('a','3'), WHITE | KNIGHT);
    addPieces(qb, toBitboard('c','3'), WHITE | PAWN);
    addPieces(qb, toBitboard('f','3'), WHITE | KNIGHT);
    addPieces(qb, toBitboard('h','3'), WHITE | PAWN);

    addPieces(qb, toBitboard('f','2'), WHITE | KING);
    addPieces(qb, toBitboard('g','2'), WHITE | PAWN);

    addPieces(qb, toBitboard('a','1'), WHITE | ROOK);
    addPieces(qb, toBitboard('g','1'), WHITE | ROOK);

    print("Disappearing King Test\n");
    printQBUnicode(b.quad);

    //
    // MOVELIST
    //
    analysisList moves;
    moves.ix = 0;
    generateLegalMoveList(&b, &moves, 0);   
    printMoveList(&moves);
    
    //
    // LOOP DETECT
    //
    board loopDetect;
    quadboard* ld = &(loopDetect.quad);
    clearBoard(&loopDetect);    
    loopDetect.piecesMoved = ~1;
    loopDetect.currentCastlingRights = ~1;
    loopDetect.whosTurn = WHITE;
    
    addPieces(ld, toBitboard('a','8'), BLACK | ROOK);
    addPieces(ld, toBitboard('g','8'), BLACK | ROOK);
    addPieces(ld, toBitboard('h','8'), BLACK | KING);

    addPieces(ld, toBitboard('a','7'), BLACK | PAWN);
    addPieces(ld, toBitboard('b','7'), BLACK | PAWN);
    addPieces(ld, toBitboard('d','7'), BLACK | BISHOP);
    addPieces(ld, toBitboard('g','7'), BLACK | PAWN);
    addPieces(ld, toBitboard('h','7'), BLACK | PAWN);
    
    addPieces(ld, toBitboard('c','6'), BLACK | KNIGHT);

    addPieces(ld, toBitboard('d','5'), BLACK | KNIGHT);

    addPieces(ld, toBitboard('a','4'), WHITE | PAWN);
    addPieces(ld, toBitboard('b','4'), WHITE | PAWN);
    addPieces(ld, toBitboard('f','4'), BLACK | PAWN);
    addPieces(ld, toBitboard('h','4'), WHITE | BISHOP);

    addPieces(ld, toBitboard('a','3'), WHITE | KNIGHT);
    addPieces(ld, toBitboard('c','3'), WHITE | PAWN);
    addPieces(ld, toBitboard('d','3'), WHITE | BISHOP);
    addPieces(ld, toBitboard('f','3'), WHITE | KNIGHT);
    addPieces(ld, toBitboard('h','3'), WHITE | PAWN);

    addPieces(ld, toBitboard('f','2'), WHITE | KING);
    addPieces(ld, toBitboard('g','2'), WHITE | PAWN);

    addPieces(ld, toBitboard('a','1'), WHITE | ROOK);
    addPieces(ld, toBitboard('g','1'), WHITE | ROOK);

    print("LOOP DETECT\n");
    printQBUnicode(loopDetect.quad);
    


    board next;
    aiMove(&b, &next, &loopDetect, 44, 0);
    
}


void testSuite() {
    board b;

    // TEST CLEARBOARD
    clearBoard(&b);
    if (b.quad.team | b.quad.type2 | b.quad.type1 | b.quad.type0) {
        error("clearBoard failed\n");
    }
    
    // TEST GET/SET PIECE
    for (byte p = PAWN; p <= KING; p += 2) {
        clearBoard(&b);
        addPieces(&(b.quad), 1ULL, WHITE | p);
        bitboard bb = getPieces(b.quad, WHITE | p);
        if (bb != 1ULL) {
            print("get/setPieces failed when adding piece %u\n", WHITE | p);
            printf("Showing piece type\n"); printByte(WHITE | p);
            printf("Showing type0\n"); printBB(b.quad.type0);
            printf("Showing type1\n"); printBB(b.quad.type1);
            printf("Showing type2\n"); printBB(b.quad.type2);
            printf("Showing team\n"); printBB(b.quad.team);
            printf("Showing getPieces result\n"); printBB(bb);
            
            exit(1);
        }
    }   
    
    // TEST GENERATE LEGAL MOVE LIST
    analysisList moveList;
    moveList.ix = 0;
    clearBoard(&b);
    initBoard(&b);
    generateLegalMoveList(&b, &moveList, 1); // Leaf Mode = 1
    if (moveList.ix != 20) {
        print("generateLegalMoveList failed, incorrect number of legal moves: %u\n", moveList.ix);
        printMoveList(&moveList);
        print("Current castling rights: ");
        printByte(b.currentCastlingRights);
        print("\n");
        exit(1);
    }
    
/*  
    // TEST LEGAL MOVES FOR EACH PIECE
    clearBoard(&b);
    initBoard(&b);
    analysisMove move;
    move.from = 16ULL;
    move.to = 1ULL << 35;
    move.promoteTo = 0;
    board bNew;
    makeMove(&b, &bNew, &move);
    bitboard enemies = getTeamPieces(bNew.quad,BLACK);
    bitboard friends = getTeamPieces(bNew.quad,WHITE);
    print("allowed queen moves\n");
    printBB(generateQueenMoves(1ULL << 35, enemies, friends)); 
    print("Allowed west moves\n");
    printBB(applySlidingAttackVector(1ULL << 35, w, enemies, friends, DIRECTION_UP));
    print("Allowed east moves\n");
    printBB(applySlidingAttackVector(1ULL << 35, w, enemies, friends, DIRECTION_DOWN));
*/  
    
    testDisappearingKing();
    
}
