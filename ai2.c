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
int aiStrength;

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
	amv->mv.castlingMove = mv.castlingMove;
	amv->mv.promoteTo = mv.promoteTo;
	memcpy((void*)&(amv->mv.resultingBoard), (void*)&(mv.resultingBoard), sizeof(board));
	amv->score = score;

//	printf("DONE\n");
//	fflush(stdout);

}

//
// Add an analysis move to the analysis movelist, with bounds checking.
//
void addAnalysisMove(analysisList* amvs, analysisMove amv2, int numMoves) {

//	printf("About to addAnalysisMove...");
//	fflush(stdout);
		
	int ix = aiStrength - numMoves;	
		
	if (ix < ANALYSIS_SIZE) {
		
		analysisMove* amv = &(amvs->moves[ix]);
		
		amv->mv.from.x = amv2.mv.from.x;
		amv->mv.from.y = amv2.mv.from.y;
		amv->mv.to.x = amv2.mv.to.x;
		amv->mv.to.y = amv2.mv.to.y;
		amv->mv.promoteTo = amv2.mv.promoteTo;	   
		amv->mv.castlingMove = amv2.mv.castlingMove;
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

	byte opposingTeam = opponentOf(team);
	moveList teamMoves, opponentMoves;
	
	assessMobility(&teamMoves, b, team);
	assessMobility(&opponentMoves, b, opposingTeam);

//	printf("mobility score: %d\n", teamMoves.ix - opponentMoves.ix); 

	return teamMoves.ix - opponentMoves.ix;	
}


//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "numMoves"
//
analysisList* getBestMove(analysisMove* bestMove, board* b, byte scoringTeam, int numMoves) {

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

/*
			printf("Pondering Move at depth %d: ",aiStrength - numMoves);
			printMove(b, mvs.moves[z]);
			printBoardClassic(&(mvs.moves[z].resultingBoard));
			printf("\n");
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
			history = getBestMove(&bestNextMove, &(mvs.moves[z].resultingBoard), scoringTeam, numMoves - 1);
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
					addAnalysisMove(bestHistory, *bestMove, numMoves);
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
					addAnalysisMove(bestHistory, *bestMove, numMoves);
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
		bestMove->score = evaluateMobility(b, scoringTeam) + evaluateMaterial(b, scoringTeam);	
		
		// Since this end result might be the one true path, be presumptious and allocate a new
		// move history for it.
		return (analysisList*)malloc(sizeof(analysisList));	
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
	analysisList* bestAnalysis = getBestMove(&bestmove, current, current->whosTurn, aiStrength);

	printf("Reasoning now being printed\n");
	fflush(stdout);
	int i;
	for (i = 0; i < aiStrength; i++) {
		printf("Depth %d, score %d\n", i + 1, bestAnalysis->moves[i].score);
		printBoardClassic(&(bestAnalysis->moves[i].mv.resultingBoard));
	}

	// deallocate the now useless history.
	free(bestAnalysis);

	printf("\n");
	makeMove(current, next, bestmove.mv);

	time_t finishTime = time(NULL);
	double timetaken = difftime(finishTime, startTime);
	
	printf("===== ai move for ");printTeam(current->whosTurn);printf(" =====\n");
	printf("Move chosen: ");
	printf(" ");printMove(current, bestmove.mv);printf(" (score: %d)\n",bestmove.score);
    printf("Ai Move Time Taken: %f\n", timetaken);

/*
	if (timetaken < 5) {
		aiStrength++;
		printf("Raising ai strength to %d\n", aiStrength);
	}
	else if (timetaken > 300) {
		aiStrength--;
		printf("Lowering ai strength to %d\n", aiStrength);
	}
*/

	printBoardClassic(next);
	
	fflush(stdout);	
}
