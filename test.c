void testSuite() {
	
	test_quadboard();
	
	board b;

	// TEST CLEARBOARD
	clearBoard(&b);
	if (b.quad.team | b.quad.type2 | b.quad.type1 | b.quad.type0) {
		error("clearBoard failed\n");
	}
	
	// TEST GET/SET PIECE - SINGULAR
	for (byte p = PAWN; p <= KING; p += 2) {
		clearBoard(&b);
		addPieces(&(b.quad), 1ULL, WHITE | p);
		print("Printing qb after adding piece "); printPieceUnicode(p, WHITE, UNICODESET_SOLID); print("\n");
		printResetColors();
		printQB(b.quad); print("\n");
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
		

	// TEST GET/SET PIECE - MULTIPLE
	for (byte p = PAWN; p <= KING; p += 2) {
		clearBoard(&b);
		addPieces(&(b.quad), 15ULL, WHITE | p);
		print("Printing qb after adding piece "); printPieceUnicode(p, WHITE, UNICODESET_SOLID); print("\n");
		printResetColors();
		printQB(b.quad); print("\n");
		bitboard bb = getPieces(b.quad, WHITE | p);
		if (bb != 15ULL) {
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
	
	// Examining internal state of quadboard
	clearBoard(&b);
	initBoard(&b);
	print("printing type0 (bit 3)\n");
	printBB(b.quad.type0);
	print("\n");
	print("printing type1 (bit 2)\n");
	printBB(b.quad.type1);
	print("\n");
	print("printing type2 (bit 1)\n");
	printBB(b.quad.type2);
	print("\n");
	print("printing team  (bit 0)\n");
	printBB(b.quad.team);
	print("\n");
	

	// TEST GETPIECEs - INITIAL CONFIG, EVERY PIECE TYPE AND TEAM
	clearBoard(&b);
	initBoard(&b);
	for (byte t = WHITE; t <= BLACK; t++) {
		for (byte p = PAWN; p <= KING; p += 2) {
			print("Printing only "); printPieceUnicode(p,t,UNICODESET_SOLID); print("\n");
			printResetColors();
			bitboard pieces = getPieces(b.quad, t|p);
			printBB(pieces);
			print("\n");
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
		printQBUnicode(b.quad);
		
		print("\ngetTeamPieces WHITE \n");
		printBB(getTeamPieces(b.quad, WHITE));
		print("\n");

		print("getTeamPieces BLACK \n");
		printBB(getTeamPieces(b.quad, BLACK));
		print("\n");
		
		printMoveList(&moveList);
		exit(1);
	}
	
	
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
	
}
