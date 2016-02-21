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

typedef struct {
	byte x;
	byte y;
	byte promoteTo;
	byte checkingMove;
	byte castlingMove;
} action;

typedef struct {
	byte piece;
	square from;
	square to;
	byte promoteTo;
	byte checkingMove;
	byte castlingMove;
} move;

typedef struct {
	action actions[ACTIONS_SIZE];
	byte ix;
} actionList; 

typedef struct {
	move moves[MOVES_SIZE];
	int ix;
} moveList;

//#define copyMove(copy,orig) (copy).x = (orig).x;(copy).y = (orig).y;(copy).promoteTo = (orig).promoteTo; (copy).checkingMove = (orig).checkingMove; (copy).castlingMove = (orig).castlingMove
#define lastAddedAction(acts) ((acts)->actions[(acts)->ix - 1])


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
		act->checkingMove = 1;  // default value.  Must be explicitly set by caller if different.
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
	
//	printf("Calling addMove.  moveList->ix = %d\n", mvs->ix);
//	fflush(stdout);
	
	
	if (mvs->ix < MOVES_SIZE) {
		move* mv = &(mvs->moves[mvs->ix]);
		mv->from.x = from.x;
		mv->from.y = from.y;
		mv->to.x = act.x;
		mv->to.y = act.y;
		mv->promoteTo = act.promoteTo;	   
		mv->checkingMove = act.checkingMove;
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
// Note that if the destination square is occupied by another piece of the same team, the move
// will not be added.
//
void addUnblockableSquare(actionList* mvs, board* b, square from, int x, int y) {
	if (x >= 0 && x <= 7 && y >= 0 && y <= 7) {
		byte mover = boardAtSq(b,from);
		byte victim = boardAt(b,x,y);
		if (victim == 0 || teamOf(victim) != teamOf(mover)) { 
			// We have found a square that is not occupied by a teammate piece, so add it to the list.
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
			if (teamOf(victim) != teamOf(mover)) {
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
void addPawnAction(actionList* mvs, byte x, byte y, int lastrank, byte checkingMove) {
	
	if (y == lastrank) {
		//
		// Pawn promotion logic pt1 (makes the promotion into 4 options for moving)
		//
		// the actual transformation of the pawn happens in makeMove (not addMove)
		//
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = QUEEN;  lastAddedAction(mvs).checkingMove = checkingMove;
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = ROOK;   lastAddedAction(mvs).checkingMove = checkingMove;
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = BISHOP; lastAddedAction(mvs).checkingMove = checkingMove;
		addAction(mvs,x,y); lastAddedAction(mvs).promoteTo = KNIGHT; lastAddedAction(mvs).checkingMove = checkingMove;
	}
	else {
		addAction(mvs,x,y);
		lastAddedAction(mvs).checkingMove = checkingMove;
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
					addPawnAction(mvs,x,y+forward,lastrank,0);
					
					// If the square ahead is clear, and it's first pawn move, 
					// then add the move 2 squares distant (if that square is also vacant.)
					if (y == firstrank && boardAt(b,x,y+(2*forward)) == 0) {
						addPawnAction(mvs,x,y+(2*forward),lastrank,0);
					}
					
				}
				
				// if an enemy piece is present, can move diagonally to take.
				if (x < 7) {
					byte p = boardAt(b,x+1,y+forward);
					if (p > 0 && teamOf(p) == opponentOf(team)) {
						addPawnAction(mvs,x+1,y+forward,lastrank,1);
					}
				}
				if (x > 1) {
					byte p = boardAt(b,x-1,y+forward);
					if (p > 0 && teamOf(p) == opponentOf(team)) {
						addPawnAction(mvs,x-1,y+forward,lastrank,1);
					}
				}
			}
			
			break;
			
		default:
			printf("\nallowedActions() called for unknown piece type\n");
	}
}	

//
// Take all the actions of a piece and add it to the overall movelist for the board.
//
void addActionListToMoveList(actionList* acts, square from, moveList* mvs) {
	for (int i = 0; i < acts->ix; i++) {
		addMove(mvs,from,acts->actions[i]);
	}
}

//
// Build a list of allowedMoves for a team on board b.
//
// Uses the team specified in parameter, rather than b->whosTurn.
// This allows for calculating checked squares as well as building move lists.
//
void allowedMoves(moveList* mvs, board* b, byte team) {

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
				addActionListToMoveList(&acts, from, mvs);
			}
		}
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

void printAllowedMoves(board* b) {
	moveList mvs;
	allowedMoves(&mvs,b,b->whosTurn);
	for (int i = 0; i < mvs.ix; i++) {
		printMove(b, mvs.moves[i]);
	}
	printf("\n");
}


