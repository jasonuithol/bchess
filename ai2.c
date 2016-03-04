// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//

//
// NOTE: GLOBAL VARIABLES
//



byte queenCanMove;

//#define lastAddedAnalysis(list) ((list)->moves[(list)->ix - 1])


//
// Add an analysis move to the analysis movelist, with bounds checking.
//

scoreType analyseNonLeafMove(const analysisMove move, const board* const b, const byte scoringTeam, const depthType numMoves, const depthType aiStrength) {

	const depthType depth = aiStrength - numMoves;

}

typedef void (moveIteratorCallbackFuncPtr)(bitboard piece, bitboard move, byte promoteTo);



//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "numMoves"
//
analysisMove getBestMove(const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth) {

	if (depth == aiStrength) {
		return analyseLeafNonTerminal(b->quad, scoringTeam);
	}
	else {
		
		// Start by assuming the worst for us (or the best for the opponent), and see if we can do better than that.
		scoreType bestScore = (b->whosTurn == scoringTeam ? -9999 : 9999);
		analysisList moveList;
		
		if (depth < aiStrength) {
			
			//
			// Do non-leaf analysis
			//
			generateLegalMoveList(b, moveList, 0);			
		
		}
		else {
			
			//
			// Do leaf analysis
			//
			generateLegalMoveList(b, moveList, 1);			
			
		}

		// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
		if (moveList.ix == 0) {
			
			if (depth == 0) {
				error("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
			} 
			else {
				return analyseLeafTerminal(b, scoringTeam, depth);
			}
		}

		
		for (byte ix = 0; ix < moveList.ix; ix++) {
			
			//
			// Assess the move [from]->[to] on board b to depth numMoves-1.
			//
			// We do this by looking at the resulting board and then seeing what score
			// we get when we follow all the moves from then on down to numMoves -1
			// and see if any of those leaf level boards has a higher best score
			// than what we have now.
			//
			scoreType score = getBestMove(&bestNextMove, &(mvs.moves[z].resultingBoard), scoringTeam, numMoves - 1, aiStrength);

			
			//
			// Update bestscore to be the best score.
			//
			if (b->whosTurn == scoringTeam) {
				// We analysed one of our moves, so pick the highest scoring move.
				if (score > bestScore) {
					
					logg("New best score found at depth %d for my move: %d\n", depth, score);
					
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
					
					// For now, only log possible alternative moves.
					if (score > bestScore - 4 && depth == 0) {
						logg("Found competing alternative to best scoring move so far for score: %d\n", score);
					}
					
				}
			}
			else {
				// We analysed one of the opponent's moves, so pick the lowest scoring move.
				// This basically assumes that the opponent will play to their best ability.
				// Note that the score is OUR score, not the moving team's score.
				if (score < bestScore) {
					
					logg("New best score found at depth %d for their move: %d\n", depth, score);
					
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

			
		} // for z




		// Return pointer to current best analysis history
		return bestMove;
		
	} // if leaf
}

