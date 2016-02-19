// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//

//
// NOTE: GLOBAL VARIABLES
//
int nodesCalculated;
int aiStrength;

typedef struct {

	square from;
	move to;
	int score;
	
} analysisMove;

//
// A strategy for deciding the best move based on how much material the player has at the end of each possible move.
//
int evaluateMaterial(board* b, byte team) {
	int score = 0;
	int x,y;
	int teamMultiplier = 0;
	for (x=0;x<8;x++) {
		for (y=0;y<8;y++) {
			byte p = boardAt(b,x,y);
			if (teamOf(p) == team) {
				teamMultiplier = 1;
			}
			else {
				teamMultiplier = -1;
			}
			switch(typeOf(p)) {
				case PAWN:   score += teamMultiplier * 10; break;
				case KNIGHT: score += teamMultiplier * 30; break;
				case BISHOP: score += teamMultiplier * 40; break;
				case ROOK:   score += teamMultiplier * 70; break;
				case QUEEN:  score += teamMultiplier * 100; break;
				case KING:   score += teamMultiplier * 1000; break;
			} 
		}
	}
	return score;
}


//
// A strategy for deciding the best move based on how many more possible moves the player has than it's opponent.
//
int evaluateMobility(board* b, byte team) {

	int x,y;
	int teamScore = 1;
	int opponentScore = 1;
	byte opposingTeam = opponentOf(team);
	
	for (x=0;x<8;x++) {
		for (y=0;y<8;y++) {
			square from = {x,y};
			byte p = boardAt(b,x,y);
			if (teamOf(p) == team) {
			
				moveList mvs;
				allowedMoves(&mvs, b, from);
				teamScore += mvs.ix;
			}
			else {
				if (teamOf(p) == opposingTeam) {
				
					moveList mvs;
					allowedMoves(&mvs, b, from);
					opponentScore += mvs.ix;
				
				}
			}
		}
	}
	return teamScore - opponentScore;
}


//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "numMoves"
//
void getBestMove(analysisMove* bestMove, board* b, byte scoringTeam, int numMoves) {

	if (numMoves > 0) {

		// Start by assuming the worst for us (or the best for the opponent), and see if we can do better than that.
		int bestScore = (b->whosTurn == scoringTeam ? -9999 : 9999);
		
		int x,y,z;
		for (x=0;x<8;x++) {
			for (y=0;y<8;y++) {
				square from = {x,y};
				if (teamOf(boardAt(b,x,y)) == b->whosTurn) {

					//				
					// We have found a square with one of our pieces on it.  Go through it's allowed moves.
					//				
				
					moveList mvs;
					allowedMoves(&mvs, b, from);
					for (z=0;z<mvs.ix;z++) {	

						//
						// Assess the move [from]->[to] on board b to depth numMoves-1.
						// We do this by making an allowed move on a new board (copied from current board) 
						// and then seeing what score we get when we follow all the best moves from then on.
						//
						board bNext;
						makeMove(b, &bNext, from, mvs.moves[z]);
						analysisMove bestNextMove;
						getBestMove(&bestNextMove, &bNext, scoringTeam, numMoves - 1);
						int score = bestNextMove.score;

						
						//
						// Update bestscore to be the best score.
						//
						if (b->whosTurn == scoringTeam) {
							// We analysed one of our moves, so pick the highest scoring move.
							if (score > bestScore) {
								bestScore = score;
								bestMove->score = score;
								copySquare(bestMove->from,from);
								copyMove(bestMove->to,mvs.moves[z]);
							}
						}
						else {
							// We analysed one of the opponent's moves, so pick the lowest scoring move.
							// This basically assumes that the opponent will play to their best ability.
							// Note that the score is OUR score, not the moving team's score.
							if (score < bestScore) {
								bestScore = score;
								bestMove->score = score;
								copySquare(bestMove->from,from);
								copyMove(bestMove->to,mvs.moves[z]);
							}
						}
						
					} // for z
				} // if whosTurn
			} // for y
		} // for x
		
	} // if numMoves
	else {
	
		// Animate a little spinner to show that we are still alive.
		nodesCalculated = (nodesCalculated + 1) % 100000;
		switch (nodesCalculated) {
			case 0:
				printf("/\b");
				fflush(stdout);
				break;
			case 24900:
				printf("-\b");
				fflush(stdout);
				break;
			case 49900:
				printf("\\\b");
				fflush(stdout);
				break;
			case 74900:
				printf("|\b");
				fflush(stdout);
				break;
		}
	
		// We have hit the limit of our depth search - time to score the board.
		bestMove->score = evaluateMobility(b, scoringTeam) + evaluateMaterial(b, scoringTeam);
		
	}
}

//
// Ask an AI agent to make a move.
//
void aiMove(board* current, board* next) {

	time_t startTime = time(NULL);
	
	nodesCalculated = 0;
	analysisMove bestmove;
	
	// TODO: Pick an AI strategy and pass it into this method call.
	getBestMove(&bestmove, current, current->whosTurn, aiStrength);

	printf("\n");
	makeMove(current, next, bestmove.from, bestmove.to);

	time_t finishTime = time(NULL);
	
	printf("===== ai move for ");printTeam(current->whosTurn);printf(" =====\n");
	printf("Move chosen: ");printPiece(boardAtSq(current,bestmove.from));
	printf(" ");printSquare(bestmove.from);printf("->");printMove(bestmove.to);printf(" (score: %d)\n",bestmove.score);
    printf("Ai Move Time Taken: %f\n", difftime(finishTime, startTime));

	printBoardClassic(next);
	
}
