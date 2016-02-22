// ================================================================
//
// moves.c
//
// The rules for moving pieces are embodied here.
//

// TODO: Get rid of the magic numbers.

#define ACTIONS_SIZE (28)
#define MOVES_SIZE (ACTIONS_SIZE * 16)

#define KINGSIDE_CASTLE_MOVE (1)
#define QUEENSIDE_CASTLE_MOVE (2)

#define MODE_MOVES_LIST (0)
#define MODE_ATTACK_LIST (1)
#define MODE_MOBILITY_LIST (2)

#define BOARD_LEGAL (1)
#define BOARD_NOT_LEGAL (0)

#define BOARD_NORMAL (0)
#define BOARD_CHECKMATE (1)
#define BOARD_STALEMATE (2)


// NOTE: MODE_MOVES_LIST is the default mode.
// Any code switching to MODE_ATTACK_LIST absolutely must 
// switch the mode back to MODE_MOVES_LIST afterwards.
byte movesMode = MODE_MOVES_LIST;

typedef struct {
	byte x;
	byte y;
	byte promoteTo;
	byte castlingMove;
} action;

typedef struct {
	byte piece;
	square from;
	square to;
	byte promoteTo;
	byte castlingMove;
	board resultingBoard;
} move;

typedef struct {
	action actions[ACTIONS_SIZE];
	byte ix;
} actionList; 

typedef struct {
	move moves[MOVES_SIZE];
	int ix;
} moveList;

#define lastAddedAction(acts) ((acts)->actions[(acts)->ix - 1])
#define lastAddedMove(mvs) ((mvs)->moves[(mvs)->ix - 1])


//
// Move a piece from one square to another, taking into account special rules for pawn promotion and castling.
//
void makeMove(board* old, board* new, move mv) {
		
	// This creates a new board with the piece moved to the new square.
	spawnBoard(old, new, mv.from, mv.to);

	//
	// Pawn promotion logic pt2 (takes the chosen promotion and applies it)
	//
	if (mv.promoteTo > 0) {
		boardAtSq(new,mv.to) = teamOf(boardAtSq(new,mv.to)) + mv.promoteTo;
	}
	
	//
	// TODO: Use constants instead of magic numbers. 
	//
	
	// Kingside Castle cleanup - i.e. move the kingside castle to it's new spot.
	if (mv.castlingMove == KINGSIDE_CASTLE_MOVE) {
		// Castle moves from square [7, y] to [5, y].
		boardAt(new,5,mv.to.y) = boardAt(new,7,mv.to.y); 
		boardAt(new,7,mv.to.y) = 0;
	}
	// Queenside Castle cleanup - i.e. move the queenside castle to it's new spot.
	if (mv.castlingMove == QUEENSIDE_CASTLE_MOVE) {
		// Castle moves from square [0, y] to [3, y].	
		boardAt(new,3,mv.to.y) = boardAt(new,0,mv.to.y); 
		boardAt(new,0,mv.to.y) = 0;
	}
}	

//
// Add an action to the actionlist, with bounds checking.
//
void addAction(actionList* acts, byte x, byte y) {
	if (acts->ix < ACTIONS_SIZE) {
		action* act = &(acts->actions[acts->ix]);
		act->x = x;
		act->y = y;
		
		act->promoteTo = 0;		// default value.  Must be explicitly set by caller if different.
		act->castlingMove = 0;  // default value.  Must be explicitly set by caller if different.

		acts->ix = acts->ix + 1;
	}
	else {
		printf("\nMaximum actions size exceeded.\n");
		exit(EXIT_FAILURE);
	}
}


//
// Add a move to the movelist, with bounds checking.
//
void addMove(moveList* mvs, square from, action act) {
		
	if (mvs->ix < MOVES_SIZE) {
		move* mv = &(mvs->moves[mvs->ix]);
		mv->from.x = from.x;
		mv->from.y = from.y;
		mv->to.x = act.x;
		mv->to.y = act.y;
		mv->promoteTo = act.promoteTo;	   
		mv->castlingMove = act.castlingMove;
		
		mvs->ix = mvs->ix + 1;
		
	}
	else {
		printf("\nMaximum moves size exceeded.\n");
		exit(EXIT_FAILURE);
	}
	
}


//
// Add a move that cannot be blocked by an interposing piece.  Useful for Knights and Kings.
// Note that in MODE_MOVES_LIST, if the destination square is occupied by another piece of 
// the same team, the move will not be added.
//
void addUnblockableSquare(actionList* mvs, board* b, square from, int x, int y) {
	if (x >= 0 && x <= 7 && y >= 0 && y <= 7) {
		byte mover = boardAtSq(b,from);
		byte victim = boardAt(b,x,y);
		if (movesMode == MODE_ATTACK_LIST || (victim == 0 || teamOf(victim) != teamOf(mover))) { 
			// We have found a square that is not occupied by a teammate piece, so add it to the list.
			// In MODE_ATTACK_LIST we add all squares.
			addAction(mvs,x,y);
		}
	}
}

//
// Add all moves that radiate out in a particular direction, stopping when we hit a piece or the edge of the board.
//
void addBlockableSquares(actionList* mvs, board* b, square from, int xVector, int yVector) {

	int n = 1;
	int nx = from.x;
	int ny = from.y;
	byte mover = boardAtSq(b,from);
	
	while (n < 8) { // I'm pretty sure this check is useless.  
					// It's only here to stop a runaway process should we go off the edge of the board.
	
		nx += xVector;
		ny += yVector;
		if (nx < 0 || nx > 7 || ny < 0 || ny > 7) {
			// We have hit the edge of the board, hence no more moves available.
			break;
		}
		
		byte victim = boardAt(b,nx,ny);
		if (victim == 0) {
			// There is no piece sitting on this square, so add this move to the list and keep going.
			addAction(mvs, nx, ny);
		}
		else {
			// We have hit a piece.  Beyond this square there are no more moves, so no matter what, 
			// we are going to exit the loop.
			
			// However, if it's an opponent, we will add this square just before exiting.
			// This move would take the piece, so it's legit.
			// In mode MODE_ATTACK_LIST, we always add this last square.
			if (movesMode == MODE_ATTACK_LIST || teamOf(victim) != teamOf(mover)) {
				addAction(mvs, nx, ny);
			}

			// ...and, we exit.
			break;
		}
		n += 1; // Again, only here to prevent bad board edge checking from causing a runaway process.
	}
}

//
// Add a pawn move to the movelist.  We need a special method for this to take into account the ability
// of pawns to transform into another piece when it reaches the last rank.
//
void addPawnAction(actionList* mvs, byte x, byte y, int lastrank) {
	
	if (y == lastrank) {
		//
		// Pawn promotion logic pt1 (makes the promotion into 4 options for moving)
		//
		// the actual transformation of the pawn happens in makeMove (not addMove)
		//
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = QUEEN;
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = ROOK;
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = BISHOP;
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = KNIGHT;
		
		//TODO: In mode MODE_ATTACK_LIST, the attack list must include the 
		// attacks for all four of these pieces !
	}
	else {
		addAction(mvs,x,y);
	}
}

//
// Populate a list with all the allowed actions that a specific piece can make.
//
void allowedActions(actionList* mvs, board* b, square from) {

	byte team = teamOf(boardAtSq(b,from));
	byte type = typeOf(boardAtSq(b,from));
	byte x = from.x;
	byte y = from.y;
	
	mvs->ix = 0;

	// (for pawn move calculation)
	int forward;
	int firstrank; 
	int lastrank;
	
	// Step one, create basic move matrix
	switch(type) {
	
		case KING:
		
			// immediately surrounding squares
			addUnblockableSquare(mvs,b,from,x-1,y-1);
			addUnblockableSquare(mvs,b,from,x,y-1);
			addUnblockableSquare(mvs,b,from,x+1,y-1);

			addUnblockableSquare(mvs,b,from,x-1,y);
			addUnblockableSquare(mvs,b,from,x+1,y);

			addUnblockableSquare(mvs,b,from,x-1,y+1);
			addUnblockableSquare(mvs,b,from,x,y+1);
			addUnblockableSquare(mvs,b,from,x+1,y+1);


			if (movesMode == MODE_MOVES_LIST && movesMode == MODE_MOBILITY_LIST) {
			
				//
				// Make a list of all the castling options still available at this point.
				//
				int whiteQueenside = b->piecesMoved & (WHITE_QUEENSIDE_CASTLE_MOVED | WHITE_KING_MOVED);
				int blackQueenside = b->piecesMoved & (BLACK_QUEENSIDE_CASTLE_MOVED | BLACK_KING_MOVED);
				int whiteKingside = b->piecesMoved & (WHITE_KINGSIDE_CASTLE_MOVED | WHITE_KING_MOVED);
				int blackKingside = b->piecesMoved & (BLACK_KINGSIDE_CASTLE_MOVED | BLACK_KING_MOVED);
						
				if ((team == WHITE && whiteQueenside == 0) || (team == BLACK && blackQueenside == 0)) {

					//
					// Able to castle now, but first check for ugly stuff.
					//
					
					// Check that the intervening squares are clear of pieces.
					if (boardAt(b,1,y) == 0 && boardAt(b,2,y) == 0 && boardAt(b,3,y) == 0) {
					
						// TODO: uh oh, need the opposition's allowedMoves to scan for attacked squares....
						
						// makeMove (but not addMove) will recognise a King moving two squares and will take care of moving the castle for us.
						// The castle doesn't need to move when this move is being added to list of moves,
						// but will need to move should the move actually be played on a board.
						addAction(mvs,2,y);
						lastAddedAction(mvs).castlingMove = QUEENSIDE_CASTLE_MOVE;
					}
					
				}
				else {
					if ((team == WHITE && whiteKingside == 0) || (team == BLACK && blackKingside == 0)) {

						//
						// Able to castle now, but first check for ugly stuff.
						//
						
						// Check that the intervening squares are clear of pieces.
						if (boardAt(b,5,y) == 0 && boardAt(b,6,y) == 0) {
						
							// TODO: uh oh, need the opposition's allowedMoves to scan for attacked squares....
							
							// makeMove (but not addMove) will recognise a King moving two squares and will take care of moving the castle for us.
							// The castle doesn't need to move when this move is being added to list of moves,
							// but will need to move should the move actually be played on a board.
							addAction(mvs,6,y);
							lastAddedAction(mvs).castlingMove = KINGSIDE_CASTLE_MOVE;
						}
						
					}
				}
			}
			
			break;
			
		case QUEEN:
		
			//diagonals
			addBlockableSquares(mvs,b,from,1,1);
			addBlockableSquares(mvs,b,from,1,-1);
			addBlockableSquares(mvs,b,from,-1,1);
			addBlockableSquares(mvs,b,from,-1,-1);

			//rank and file moves
			addBlockableSquares(mvs,b,from,1,0);
			addBlockableSquares(mvs,b,from,-1,0);
			addBlockableSquares(mvs,b,from,0,1);
			addBlockableSquares(mvs,b,from,0,-1);
			break;
			
		case ROOK:
			//rank and file moves
			addBlockableSquares(mvs,b,from,1,0);
			addBlockableSquares(mvs,b,from,-1,0);
			addBlockableSquares(mvs,b,from,0,1);
			addBlockableSquares(mvs,b,from,0,-1);
			break;
			
		case BISHOP:
			//diagonals
			addBlockableSquares(mvs,b,from,1,1);
			addBlockableSquares(mvs,b,from,1,-1);
			addBlockableSquares(mvs,b,from,-1,1);
			addBlockableSquares(mvs,b,from,-1,-1);
			break;
			
		case KNIGHT:
			addUnblockableSquare(mvs,b,from,x+1,y+2);
			addUnblockableSquare(mvs,b,from,x+1,y-2);
			addUnblockableSquare(mvs,b,from,x+2,y+1);
			addUnblockableSquare(mvs,b,from,x+2,y-1);

			addUnblockableSquare(mvs,b,from,x-1,y+2);
			addUnblockableSquare(mvs,b,from,x-1,y-2);
			addUnblockableSquare(mvs,b,from,x-2,y+1);
			addUnblockableSquare(mvs,b,from,x-2,y-1);
			break;
			
		case PAWN:
		
			//
			// IMPORTANT: Pawn doesn't use the generalised blockable/unblockable square routines.
			//			  It's rules are rather elaborate and so direct calls to addMove are made.
			//
		
			if (team == WHITE) {
				forward = 1;
				firstrank = 1;
				lastrank = 7;
			}
			else {
				forward = -1;
				firstrank = 6;
				lastrank = 0;
			}
		
			// add the square ahead (if nothing is presently there).
			if (y != lastrank) {
			
				if (boardAt(b,x,y+forward) == 0) {
					addPawnAction(mvs,x,y+forward,lastrank);
					
					// If the square ahead is clear, and it's first pawn move, 
					// then add the move 2 squares distant (if that square is also vacant.)
					if (y == firstrank && boardAt(b,x,y+(2*forward)) == 0) {
						addPawnAction(mvs,x,y+(2*forward),lastrank);
					}
					
				}
				
				// if an enemy piece is present, can move diagonally to take.
				if (x < 7) {
					byte p = boardAt(b,x+1,y+forward);
					if (movesMode == MODE_ATTACK_LIST || (p > 0 && teamOf(p) == opponentOf(team))) {
						addPawnAction(mvs,x+1,y+forward,lastrank);
					}
				}
				if (x > 1) {
					byte p = boardAt(b,x-1,y+forward);
					if (movesMode == MODE_ATTACK_LIST || (p > 0 && teamOf(p) == opponentOf(team))) {
						addPawnAction(mvs,x-1,y+forward,lastrank);
					}
				}
			}
			
			break;
			
		default:
			printf("\nallowedActions() called for unknown piece type %d at square [%d,%d]\n", type, from.x, from.y);
	}
}	

// Kind of retarded.  It will print the piece in the FROM square.
void printMove(board* b, move mv) {
	if (mv.castlingMove == QUEENSIDE_CASTLE_MOVE) {
		printf("O-O-O");
	}
	else if (mv.castlingMove == KINGSIDE_CASTLE_MOVE) {
		printf("O-O");
	}
	else {
		
		byte p = boardAtSq(b,mv.from);
		printPiece(p);
		
		printf(" [%d,%d] to [%d,%d]",mv.from.x, mv.from.y, mv.to.x, mv.to.y);
		
		if (mv.promoteTo > 0) {
			printf(" *** Promote to ");
			printType(mv.promoteTo);
			printf(" ***");
		}
		
	}
}

//
// Build a list of allowedMoves for a team on board b.
//
// Uses the team specified in parameter, rather than b->whosTurn.
// This allows for calculating checked squares as well as building move lists.
//
void buildMoveList(moveList* mvs, board* b, byte team, int mode) {

	// WARNING: this here is a SIDE EFFECT.
	int currentMode = movesMode;
	movesMode = mode;


	// Ensure the movelist is initialised
	mvs->ix = 0;

	int x,y;
	for (x=0;x<8;x++) { 
		for (y=0;y<8;y++) {
			square from = {x,y};
			
			if (teamOf(boardAt(b,x,y)) == team) {

				//				
				// We have found a square with one of the specified team's pieces on it.
				// Build it's allowed actions and add them to the movelist.
				//				
				actionList acts;
				allowedActions(&acts, b, from);
				for (int i = 0; i < acts.ix; i++) {
										
					// Add the move to the movelist.  This also creates the move.
					addMove(mvs,from,acts.actions[i]);

					// If we're building a moves list, then a few extra's need doing.
					if (movesMode == MODE_MOVES_LIST) {

						// Grab a pointer to the board field in the newly created move.
						board* nextBoard = &(lastAddedMove(mvs).resultingBoard);
						
						// Prespawn a board into that field, it's going to be used a few times.
						makeMove(b, nextBoard, lastAddedMove(mvs)); 
					
						// Check for illegal moves.  This involves switching modes
						// and then re-entry into buildMoveList in ATTACK mode.
						moveList attacks;
						
						// RE-ENTRANT CALL IN ATTACK MODE.
						// Get a list of opponent moves for nextBoard.
						buildMoveList(&attacks, nextBoard, opponentOf(team), MODE_ATTACK_LIST);
						
						int isLegal = BOARD_LEGAL;
						int j;
						for (j = 0; j < attacks.ix; j++) {
							byte p = boardAtSq(nextBoard, attacks.moves[j].to);
							if (typeOf(p) == KING && team == teamOf(p)) {
								// If it's not your turn, then your King cannot be in check.
			 					// Dat's da rules.
								isLegal = BOARD_NOT_LEGAL;
								break;
							}
						}

						// If the resulting board is illegal just chuck this move out.
						if (isLegal == BOARD_NOT_LEGAL) {
							mvs->ix--;
						}
						 
					}
					
				} // for i
			} // if team
		} // for y
	} // for x

	// Restore the movesMode we were in before this method was called.
	// should always restore to MODE_MOVES_LIST
	movesMode = currentMode;
}

//
// Build a list of allowedMoves for a team on board b.
//
void allowedMoves(moveList* mvs, board* b, byte team) {
	buildMoveList(mvs, b, team, MODE_MOVES_LIST);
}


void assessMobility(moveList* mvs, board* b, byte team) {
	// Skips chucking out illegal moves.  
	// Less accurate but WAY faster.
	buildMoveList(mvs, b, team, MODE_MOBILITY_LIST);
}

int detectCheckmate(board* b) {

	moveList mvs, attacks;
	allowedMoves(&mvs, b, b->whosTurn);
	
	// Checkmate/stalemate detection.
	if (mvs.ix == 0) {
		
		// Get a list of opponent moves for nextBoard.
		buildMoveList(&attacks, b, opponentOf(b->whosTurn), MODE_ATTACK_LIST);
		
		int j;
		for (j = 0; j < attacks.ix; j++) {
			byte p = boardAtSq(b, attacks.moves[j].to);
			if (typeOf(p) == KING && b->whosTurn == teamOf(p)) {
				return BOARD_CHECKMATE;
			}
		}
		return BOARD_STALEMATE;
	}
	else {
		return BOARD_NORMAL;
	}	
}

void printAllowedMoves(board* b) {
	moveList mvs;
	allowedMoves(&mvs,b,b->whosTurn);
	for (int i = 0; i < mvs.ix; i++) {
		printMove(b, mvs.moves[i]);
	}
	printf("\n");
}


