#define WHITE_QUEENSIDE_CASTLE_MOVED	(128)
#define WHITE_KING_MOVED				(64)
#define WHITE_KINGSIDE_CASTLE_MOVED		(32)
#define BLACK_QUEENSIDE_CASTLE_MOVED	(16)
#define BLACK_KING_MOVED				(8)
#define BLACK_KINGSIDE_CASTLE_MOVED		(4)

#define BOARD_LEGAL 	(1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL 	(0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)


typedef struct { // 35 bytes
	quadboard quad;
	byte currentCastlingRights; 	// Used to check castling ability for CURRENT move only.
	byte piecesMoved;  				// Used to check castling ability for all future moves.
	byte whosTurn;     				// 0 = WHITE, 1 = BLACK.
} board;


typedef struct {

	bitboard from;
	bitboard to;
	byte promoteTo;
	board resultingBoard;
	
} analysisMove;


#define ANALYSIS_SIZE (100)

typedef struct {
	
	analysisMove items[ANALYSIS_SIZE];
	byte ix;
	
} analysisList; 


void clearBoard(board* const b) {
	memset((void*)b, 0, sizeof(board));
}


inline byte isKingChecked(const quadboard qb, byte team) {
	return isSquareAttacked(qb, getPieces(qb, KING | team), team);
}

//
// PRECONDITION: Only call this if there were no legal moves to make.
//
inline byte determineEndOfGameState(const board* const b) {
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
inline byte spawnLeafBoard(const board* const old, 
					board* const new, 
					const analysisMove* const move) {


	quadboard* const qb = &(new->quad);

	// To create a new board from a current board, first copy the current board's content to the new one.
	memcpy((void*)new, (void*)old, sizeof(board));

	// Now apply the change to the new board (i.e. move the piece from square "from" to square "to").
	moveSquare(qb, move->from, move->to);

	// Change the turn on the new board.  To get the team that just moved, use old->whosTurn.
	new->whosTurn = old->whosTurn ^ 1;


	// Now we can perform the legality check
	//
	if (isKingChecked(new->quad, old->whosTurn)) {
		return BOARD_NOT_LEGAL;
	}

	
	//
	// Pawn promotion followup (takes the chosen promotion and applies it)
	//
	if (move->promoteTo > 0) {
		resetSquares(qb, move->to);
		addPieces(qb, move->to, move->promoteTo | old->whosTurn);
	}

	//
	// Castling followup (moves the castle over the king).
	//
	if (getPieces(old->quad, KING | old->whosTurn) & move->from) {
		char rank = getRank(move->from) + '1';
		if (getFile(move->from) - getFile(move->to) == 2) {
			// Kingside castle detected
			moveSquare(qb, toBitboard('h',rank), toBitboard('f',rank));
		}
		else if (getFile(move->to) - getFile(move->from) == 2) {
			// Queenside castle detected
			moveSquare(qb, toBitboard('a',rank), toBitboard('d',rank));
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
inline byte spawnFullBoard(const board* const old, 
					board* const new, 
					const analysisMove* const move) {


	// First, spawn a leaf board, it will have all the tidying up and illegal position checks.
	if (spawnLeafBoard(old, new, move) == BOARD_NOT_LEGAL) {
		
		// Board was found to be illegal, abort and notify caller.
		return BOARD_NOT_LEGAL;
	}
	
	//
	// To see if it's still possible to castle in the future, 
	// we track whether the relevant pieces have moved.
	//
	switch(move->from) {
		case toBitboard('a','1'): new->piecesMoved |= WHITE_QUEENSIDE_CASTLE_MOVED; break;
		case toBitboard('d','1'): new->piecesMoved |= WHITE_KING_MOVED; 			break;
		case toBitboard('h','1'): new->piecesMoved |= WHITE_KINGSIDE_CASTLE_MOVED; 	break;
		case toBitboard('a','8'): new->piecesMoved |= BLACK_QUEENSIDE_CASTLE_MOVED; break;
		case toBitboard('d','8'): new->piecesMoved |= BLACK_KING_MOVED; 			break;
		case toBitboard('h','8'): new->piecesMoved |= BLACK_KINGSIDE_CASTLE_MOVED; 	break;
	}

	const bitboard allPieces = getAllPieces(new->quad);

	//
	// We need this for checking to see if in the next move, a castling move is allowed
	//
	// NOTE: FOR LEAF BOARDS, THIS IS A NEEDLESS COST !!!!!
	//
	new->currentCastlingRights = new->piecesMoved;
	
	char rank = new->whosTurn ? '8' : '1';

	//
	// KINGSIDE
	//
											
	// Check if squares are occupied.	
	if (allPieces & (toBitboard('f',rank) | toBitboard('g',rank) ) ) {

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
	if (allPieces & (toBitboard('b',rank) | toBitboard('c',rank) | toBitboard('d',rank) ) ) {

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

void addMoveIfLegal(	analysisList* const list, 
						const board* const old, 
						const analysisMove* const move,
						const byte leafMode) {

	if (list->ix < ANALYSIS_SIZE) {
		
		analysisMove* const next = &(list->items[list->ix]);
		
		next->from      = move->from;
		next->to        = move->to;
		next->promoteTo = move->promoteTo;	   
		
		const byte legality = leafMode 
							? spawnLeafBoard(old, &(next->resultingBoard), move)
							: spawnFullBoard(old, &(next->resultingBoard), move);

		if (legality == BOARD_LEGAL) {
			// Keep this board.
			list->ix++;
		}
		
		return;
	}
	else {
		error("\nMaximum analysis moves size exceeded.\n");
	}
}

