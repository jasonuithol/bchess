typedef bitboard (*funcPieceTypeMoveGen)(	const attackContext* const ac, 
											const vectorsContext* const vc, 
											const board* const b			);

typedef void (*funcPieceTypeMoveAdd)(	analysisList* const list, 
										const board* const old, 
										const analysisMove* const move,
										const byte leafMode		);

typedef struct {

	funcPieceTypeMoveGen moveGenerator;
	funcPieceTypeMoveAdd moveAdder;

	byte pieceType;
	
} moveContext;

const moveContext moveContexts[7] = {

	//
	// IMPORTANT: Order of elements determines order of execution and therefore
	//			  has major impact on pipelining and branch prediction.
	//

	{ pawnMoveGenerator, 	pawnMoveAdder,	PAWN 	},
	{ kingMoveGenerator, 	addMoveIfLegal,	KING 	},
	{ pieceMoveGenerator, 	addMoveIfLegal,	KNIGHT	},
	{ pieceMoveGenerator,	addMoveIfLegal,	ROOK	},
	{ pieceMoveGenerator,	addMoveIfLegal,	BISHOP	},
	{ pieceMoveGenerator,	addMoveIfLegal,	QUEEN	}			

};

inline void generateLegalMoveList(	const board* const b, 
							analysisList* const moveList, 
							const byte leafMode) {
	
	attackContext ac;
	
	ac.hardBlockers = getTeamPieces(b->quad, b->whosTurn);
	ac.softBlockers = getTeamPieces(b->quad, b->whosTurn ^ 1);

	for (byte i = 0; i < 6; i++) {

		// Choose the appropriate vector and moves context for the piece type (i)
		// NOTE: i does NOT map to pieceType !
		const vectorsContext* vc = &(pieceVectorContexts[b->whosTurn][i]);
		const moveContext* mc = &(moveContexts[i]);
		
		// Get all the pieces for the pieceType (e.g. all the black bishops).
		iterator pieces = newIterator(getPieces(b->quad, vc->pieceType | b->whosTurn));
		pieces = getNextItem(pieces);
					
		while (pieces.item) {
			
			ac.piece = pieces.item;

			// Obtain a list of standard attacks for the piece.												
			iterator moves = newIterator(mc->moveGenerator(&ac, vc, b));

			moves = getNextItem(moves);
			
			while (moves.item) {

				// Build a candidate move.
				const analysisMove theMove = {
					pieces.item,
					moves.item,
					0 // A DUMMY VALUE - WILL GET OVERRIDDEN IN PAWN MOVE ADDER IF PROMOTABLE
				};

				//
				// Give the pawn implemenation a chance to add
				// all the pawn promotion variants.
				//
				// Otherwise, just add the move to the list (if legal).
				//								
				mc->moveAdder(moveList, b, &theMove, leafMode);

				// Obtain the next move (if any)
				moves = getNextItem(moves);
			}
			
			// Get the next black bishop.
			pieces = getNextItem(pieces);
		}	
	}	
}




// ==========================================================
//
// 						WRAPPER FUNCTIONS
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
	return spawnFullBoard(old,new,move);
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
							   | BLACK_QUEENSIDE_CASTLE_MOVED ;
							   
	b->whosTurn = WHITE;
}

