// ================================================================
//
// ai2.c
//
// An algorithm for a computer chess player is implemented here.
//

//
// NOTE: GLOBAL VARIABLES
//


typedef uint8_t depthType;

byte queenCanMove;



//#define lastAddedAnalysis(list) ((list)->moves[(list)->ix - 1])


//
// Add an analysis move to the analysis movelist, with bounds checking.
//
void addAnalysisMove(analysisList* list, analysisMove move, depthType depth) {

	if (depth < ANALYSIS_SIZE) {
		
		analysisMove* next = &(list->moves[depth]);
		
		next->from      = move.from;
		next->to        = move.to;
		next->score     = move.score;
		next->promoteTo = move.promoteTo;	   
		
//		memcpy((void*)&(amv->mv.resultingBoard), (void*)&(amv2.mv.resultingBoard), sizeof(board));
		
	}
	else {
		error("\nMaximum analysis moves size exceeded.\n");
	}

}


scoreType analyseNonLeafMove(const analysisMove move, const board* const b, const byte scoringTeam, const depthType numMoves, const depthType aiStrength) {

	const depthType depth = aiStrength - numMoves;

}

typedef void (*moveIteratorCallbackFuncPtr)(bitboard piece, bitboard move, byte promoteTo);

void moveIterator(board* b, moveIteratorCallbackFuncPtr callback) {

	void iteratePieceTypeMoves(getterFuncPtr getter, generatorFuncPtr generator) {

		bitboard pieces = getter(b->quad, b->whosTurn);
		iterator piece = { 0ULL, pieces };
		piece = getNextItem(piece);
			
		while (piece.item) {
			
			bitboard moves = generator(piece.item, enemies, friends, b->whosTurn);
			iterator move = { 0ULL, moves };
			move = getNextItem(move);

			while (move.item) {
				callback(piece.item, move.item, 0);
				move = getNextItem(move);
			}
			
			piece = getNextItem(piece);
		}
	}

	// -------------------------------------------------------------------
	//
	// Outer Method Starts Here.
	//
	
	bitboard friends = getFriends(b->quad, b->whosTurn);
	bitboard enemies = getEnemies(b->quad, b->whosTurn);
	
	iteratePieceTypeMoves(getRooks, generatePawnMoves);
	iteratePieceTypeMoves(getKnights, generatePawnMoves);
	iteratePieceTypeMoves(getBishops, generatePawnMoves);
	iteratePieceTypeMoves(getQueens, generatePawnMoves);

	//
	// King is a special case
	//
	{
		bitboard king = getKings(b->quad, b->whosTurn);

		bitboard moves = generateKingMoves(king, enemies, friends, b->castlingCheckingMap, b->piecesMoved, b->whosTurn);
		iterator move = { 0ULL, moves };
		move = getNextItem(move);

		while (move.item) {
			callback(piece, move, 0);
		}
	}
	
	// ================= { I am trying to isolate the scope of variables } ===============
	
	
	//
	// Pawns are a special case
	//
	{
		bitboard pieces = getter(b->quad, b->whosTurn);
		iterator piece = { 0ULL, pieces };
		piece = getNextItem(piece);
			
		while (piece.item) {
			
			bitboard moves = generator(piece.item, enemies, friends, b->whosTurn);
			iterator move = { 0ULL, moves };
			move = getNextItem(move);

			while (move.item) {
				
				if (isPawnPromotable(move)) {
					callback(piece.item, move.item, QUEEN);
					callback(piece.item, move.item, KNIGHT);
					callback(piece.item, move.item, BISHOP);
					callback(piece.item, move.item, ROOK);
				}
				else {
					callback(piece.item, move.item, 0);
				}
				move = getNextItem(move);
			}
			piece = getNextItem(piece);
		}
	}
		
}


//
// Recursively search for the best move and set "bestMove" to point to it.
// Search depth set by "numMoves"
//
analysisMove getBestMove(const board* const b, const byte scoringTeam, const depthType aiStrength, const depthType depth) {

	if (depth == aiStrength) {
		return analyseLeafNonTerminal(b->quad, scoringTeam);
	}
	else {

		byte legalMoveCount = 0;
		
		// Start by assuming the worst for us (or the best for the opponent), and see if we can do better than that.
		scoreType bestScore = (b->whosTurn == scoringTeam ? -9999 : 9999);
		analysisMove bestNextMove;
		
		// move iterator start
			
			board new;
			if (depth < aiStrength) {
				
				spawnFullBoard(b, &new, piece, move, 0);
			
				//
				// Do non-leaf analysis
				//
			
			}
			else {
				
				//
				// Do leaf analysis
				//
				
			}
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


		// Checkmate/stalemate detection for AI.  Game over decision made elsewhere.
		if (legalMoveCount == 0) {
			
			if (depth == 0) {
				error("OOOPSSSS !!!! I've been asked to move when the game has finished !!!\n");
				__builtin_unreachable();
			} 
			else {
				return analyseLeafTerminal(b, scoringTeam, depth);
			}
			__builtin_unreachable();
		}


		// Return pointer to current best analysis history
		return bestMove;
		
	} // if leaf
}

