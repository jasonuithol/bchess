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
		exit(1);
	}
}