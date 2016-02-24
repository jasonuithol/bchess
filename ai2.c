// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//

//
// NOTE: GLOBAL VARIABLES
//
#define ANALYSIS_SIZE (10)


int nodesCalculated;
int queenCanMove;

typedef struct {

	move mv;
	int score;
	
} analysisMove;

typedef struct {
	
	analysisMove moves[ANALYSIS_SIZE];
	byte ix;
	
} analysisList; 

#define lastAddedAnalysis(amvs) ((amvs)->moves[(amvs)->ix - 1])

// Convert a move into an analysisMove.
void populateAnalysisMove(analysisMove* amv, move mv, int score) {
	
//	printf("About to populateAnalysisMove...");
//	fflush(stdout);
	
	amv->mv.from.x = mv.from.x;
	amv->mv.from.y = mv.from.y;
	amv->mv.to.x = mv.to.x;
	amv->mv.to.y = mv.to.y;
//	amv->mv.castlingMove = mv.castlingMove;
	amv->mv.promoteTo = mv.promoteTo;
	memcpy((void*)&(amv->mv.resultingBoard), (void*)&(mv.resultingBoard), sizeof(board));
	amv->score = score;

//	printf("DONE\n");
//	fflush(stdout);

}

//
// Add an analysis move to the analysis movelist, with bounds checking.
//
void addAnalysisMove(analysisList* amvs, analysisMove amv2, int depth) {

//	printf("About to addAnalysisMove...");
//	fflush(stdout);
				
	if (depth < ANALYSIS_SIZE) {
		
		analysisMove* amv = &(amvs->moves[depth]);
		
		amv->mv.from.x = amv2.mv.from.x;
		amv->mv.from.y = amv2.mv.from.y;
		amv->mv.to.x = amv2.mv.to.x;
		amv->mv.to.y = amv2.mv.to.y;
		amv->mv.promoteTo = amv2.mv.promoteTo;	   
//		amv->mv.castlingMove = amv2.mv.castlingMove;
		amv->score = amv2.score;
		memcpy((void*)&(amv->mv.resultingBoard), (void*)&(amv2.mv.resultingBoard), sizeof(board));
		
	}
	else {
		printf("\nMaximum analysis moves size exceeded.\n");
		exit(EXIT_FAILURE);
	}

//	printf("DONE\n");
//	fflush(stdout);
	
}


// Animates a little spinner to show that we are still alive.
void displaySpinningPulse() {

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
}


//
// A strategy for deciding the best move based on how much material the player has at the end of each possible move.
//

int pieceScore(byte p, int includeKing) {
	switch(typeOf(p)) {
		case PAWN:   return 1; break;
		case KNIGHT: return 4; break;
		case BISHOP: return 6; break;
		case ROOK:   return 14; break;
		case QUEEN:  return 21; break;
		
		case KING:   
			if (includeKing != 0) {
				return 10;  // This scores "check" in evaluateInitiative()
			}
			else {
				return 0;
			}
			break;
			
		default: return 0;
	}
}

int evaluateMaterial(board* b, byte team, int mode) {
	int score = 0;
	int x,y;
	int teamMultiplier = 0;
	byte p;
	for (x=0;x<8;x++) {
		for (y=0;y<8;y++) {
			p = boardAt(b,x,y);
			if (teamOf(p) == team) {
				teamMultiplier = 1;
			}
			else {
				if (mode == 0) {
					teamMultiplier = -1;
				}
				else {
					teamMultiplier = 0;
				}					
			}
			score += teamMultiplier * pieceScore(p, 0);
//				printf("teamMultiplier %d\n", teamMultiplier);
//				printf("piece %d, score %d\n", p, score);
		}
	}
	return score;
}


//
// A strategy for deciding the best move based on how many more possible moves the player has than it's opponent.
//
int evaluateMobility(board* b, byte team) {

	byte opposingTeam = opponentOf(team);
	moveList teamMoves, opponentMoves;
	
	assessMobility(&teamMoves, b, team);
	assessMobility(&opponentMoves, b, opposingTeam);

//	printf("mobility score: %d\n", teamMoves.ix - opponentMoves.ix); 

	return teamMoves.ix - opponentMoves.ix;	
}

//
// A strategy for deciding the best move based on initiative 
// (i.e. how much potential material gain exists statically on the board)
//
int evaluateInitiative(board* b, byte team) {
	int score = 0;
	int x,y,i;
	int teamMultiplier = 0;
	byte p,q;
	square from;
	for (x=0;x<8;x++) {
		for (y=0;y<8;y++) {
			p = boardAt(b,x,y);
			if (p > 0) {
				if (teamOf(p) == team) {
					teamMultiplier = 1;
				}
				else {
					teamMultiplier = -1;
				}
				from.x = x;
				from.y = y;
				actionList attacks; 
				allowedActions(&attacks, b, from, MODE_ATTACK_LIST);
				for (i=0;i<attacks.ix; i++) {
					byte q = boardAt(b, attacks.actions[i].x, attacks.actions[i].y);
					if (q > 0 && teamOf(p) == opponentOf(teamOf(q))) {
						score += teamMultiplier * pieceScore(q, 1);
					}
				}
			}
		}	
	}
	return score;
}



//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "numMoves"
//
analysisList* getBestMove(analysisMove* bestMove, board* b, byte scoringTeam, int numMoves, int aiStrength) {

	int depth = aiStrength - numMoves;

	if (numMoves > 0) {

		// Start by assuming the worst for us (or the best for the opponent), and see if we can do better than that.
		int bestScore = (b->whosTurn == scoringTeam ? -9999 : 9999);

		// Build a list of allowed moves for the team whose turn it is.
		moveList mvs;
		allowedMoves(&mvs, b, b->whosTurn);
		
		// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
		if (mvs.ix == 0) {
			
			if (b->whosTurn == scoringTeam) {
				
				if (aiStrength == numMoves) {
					printf("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
					exit(1);
				} 
				
				int boardState = detectCheckmate(b);
				if (boardState == BOARD_CHECKMATE) {
//					printf("Detected possible checkmate defeat at depth %d\n",aiStrength - numMoves);
//					printBoardClassic(b);
					bestMove->score = -9998 + (aiStrength - numMoves);
				}
				else {
//					printf("Detected possible stalemate at depth %d\n",aiStrength - numMoves);
//					printBoardClassic(b);
					bestMove->score = 0;
				}
			}
			else {
				int boardState = detectCheckmate(b);
				if (boardState == BOARD_CHECKMATE) {
//					printf("Detected possible checkmate victory at depth %d\n",aiStrength - numMoves);
//					printBoardClassic(b);
					bestMove->score = 9998 - (aiStrength - numMoves);
				}
				else {
//					printf("Detected possible stalemate at depth %d\n",aiStrength - numMoves);
//					printBoardClassic(b);
					bestMove->score = 0;
				}
			}
			
			// Since this end result might be the one true path, be presumptious and allocate a new
			// move history for it.
			return (analysisList*)malloc(sizeof(analysisList));	
		}

		//
		// Not a checkmate/stalemate, so search the moves for a best move.
		//

		int z;
		analysisList* bestHistory = NULL;
		analysisList* history;
		for (z=0;z<mvs.ix;z++) {	

			if (queenCanMove == 0) {
				if (typeOf(boardAtSq(b, mvs.moves[z].from)) == QUEEN) {
					continue;
				}
			}

/*
			printf("Pondering Move at depth %d: ",aiStrength - numMoves);
			printMove(b, mvs.moves[z]);
			printBoardClassic(&(mvs.moves[z].resultingBoard));
			printf("\n");e
			fflush(stdout);
*/

			//
			// Assess the move [from]->[to] on board b to depth numMoves-1.
			//
			// We do this by looking at the resulting board and then seeing what score
			// we get when we follow all the moves from then on down to numMoves -1
			// and see if any of those leaf level boards has a higher best score
			// than what we have now.
			//
			analysisMove bestNextMove;
			history = getBestMove(&bestNextMove, &(mvs.moves[z].resultingBoard), scoringTeam, numMoves - 1, aiStrength);
			int score = bestNextMove.score;

/*
			printf("FINISHED Pondering Move at depth %d: ",aiStrength - numMoves);
			printMove(b, mvs.moves[z]);
			printBoardClassic(&(mvs.moves[z].resultingBoard));
			printf("\n");
			fflush(stdout);
*/
			
			//
			// Update bestscore to be the best score.
			//
			if (b->whosTurn == scoringTeam) {
				// We analysed one of our moves, so pick the highest scoring move.
				if (score > bestScore) {
					bestScore = score;
					populateAnalysisMove(bestMove, mvs.moves[z], score);

					// Chuck out the old history
					if (bestHistory != NULL) {
						free(bestHistory);
					}
					bestHistory = history;

					// Pop the move into the new best analysis history.
					addAnalysisMove(bestHistory, *bestMove, depth);
				}
				else {
					// deallocate the now defunct analysis history
					free(history);
				}
			}
			else {
				// We analysed one of the opponent's moves, so pick the lowest scoring move.
				// This basically assumes that the opponent will play to their best ability.
				// Note that the score is OUR score, not the moving team's score.
				if (score < bestScore) {
					bestScore = score;
					populateAnalysisMove(bestMove, mvs.moves[z], score);

					// Chuck out the old history
					if (bestHistory != NULL) {
						free(bestHistory);
					}
					bestHistory = history;

					// Pop the move into the new best analysis history.
					addAnalysisMove(bestHistory, *bestMove, depth);
				}
				else {
					// deallocate the now defunct analysis history
					free(history);
				}
			}

/*
			printf("FINISHED COMPARING Move at depth %d: ",aiStrength - numMoves);
			printMove(b, mvs.moves[z]);
			printBoardClassic(&(mvs.moves[z].resultingBoard));
			printf("\n");
			fflush(stdout);
*/
			
		} // for z

		// Return pointer to current best analysis history
		return bestHistory;
		
	} // if numMoves
	else {
	
		// Tell the world we still live.
		displaySpinningPulse();
	
		// We have hit the limit of our depth search - time to score the board.
		bestMove->score =   (2 * evaluateMobility(  b, scoringTeam))
						  + (4 * evaluateMaterial(  b, scoringTeam, 0));	
						  + (1 * evaluateInitiative(b, scoringTeam));
						  
		// Since this end result might be the one true path, be presumptious and allocate a new
		// move history for it.
		return (analysisList*)malloc(sizeof(analysisList));	
	}
}

int determineAiStrength(board* current) {

	moveList myMoves, theirMoves;
	
	int n = evaluateMaterial(current,current->whosTurn, 1) 
			+ evaluateMaterial(current,opponentOf(current->whosTurn), 1);
	// Maximum material is 92	
		
	printf("Material score: %d\n", n);	
		
	if (n >= 50) {
		return 4;
	}
	else if (n >= 30) {
		return 5;
	}
	else if (n >= 15) {
		return 6;
	}
	else if (n >= 5) {
		return 7;
	}
	else {
		return 8;
	}
}

//
// Ask an AI agent to make a move.
//
void aiMove(board* current, board* next, int turnNumber) {

	time_t startTime = time(NULL);

	// I'm just sick of the queen based games.
	if (turnNumber > 5) {
		queenCanMove = 1;
	}
	else {
		queenCanMove = 0;
	}
	
	nodesCalculated = 0;
	analysisMove bestmove;
	int aiStrength = determineAiStrength(current);
			
	printf("Choosing aiStrength %d\n", aiStrength);
	fflush(stdout);
	
	// TODO: Pick an AI strategy and pass it into this method call.
	analysisList* bestAnalysis = getBestMove(&bestmove, current, current->whosTurn, aiStrength, aiStrength);

	printf("Reasoning now being printed\n");
	fflush(stdout);
	int i;
	for (i = 0; i < aiStrength; i++) {
		int mat, mob, ini;
		board* r;
		r = &(bestAnalysis->moves[i].mv.resultingBoard);
		mat = evaluateMaterial(r, current->whosTurn, 0);
		mob = evaluateMobility(r, current->whosTurn);
		ini = evaluateInitiative(r, current->whosTurn);
		printf("Depth %d, material %d, mobility %d, initiative %d\n", i + 1, mat, mob, ini);
		printBoardClassic(r);
		if (detectCheckmate(r)) {
			printf("CHECKMATE/STALEMATE\n");
			break;
		}
	}

	// deallocate the now useless history.
	free(bestAnalysis);

	printf("\n");
	makeMove(current, next, bestmove.mv);

	time_t finishTime = time(NULL);
	double timetaken = difftime(finishTime, startTime);
	
	printf("===== ai move for ");printTeam(current->whosTurn);printf(" at ai strength %d =====\n", aiStrength);
	printf("Move chosen: ");
	printf(" ");printMove(current, bestmove.mv);printf(" (score: %d)\n",bestmove.score);
    printf("Ai Move Time Taken: %f\n", timetaken);


	printBoardClassic(next);
	
	fflush(stdout);	
}
