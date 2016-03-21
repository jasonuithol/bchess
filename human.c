//
// Ask a human agent to make a move.
//

#define COMMAND_MOVE (0)
#define COMMAND_PRINTMOVES (1)
#define COMMAND_UNDO (2)
#define COMMAND_LOGGING (3)

board undoBoard;

int inputMove(analysisMove* parsedMove) {
	
   char buffer[20];
	
   print("enter your move (e.g. a1-b2): ", stdout);
   if (fgets(buffer, sizeof(buffer), stdin) != NULL) {

		switch (buffer[0]) {
			case 'M': return COMMAND_PRINTMOVES; break;
			case 'U': return COMMAND_UNDO; break;
			case 'L': return COMMAND_LOGGING; break;
		}

		parsedMove->from = toBitboard(buffer[0], buffer[1]);
		parsedMove->to   = toBitboard(buffer[3], buffer[4]);
   }
   else {
	   error("fgets() is no better than anything else around\n");
   }
   
   return COMMAND_MOVE;	
}

int inputPawnPromotion() {
	
	char buffer[20];
	
	print("Please enter which piece to promote pawn to (q,r,b,n): ", stdout);
	if (fgets(buffer, sizeof(buffer), stdin) != NULL) {

		switch (buffer[0]) {
			case 'q': return QUEEN; break;
			case 'r': return ROOK; break;
			case 'b': return BISHOP; break;
			case 'n': return KNIGHT; break;
		}
	}
	else {
	   error("fgets() is no better than anything else around\n");
	}

	return 0;	// This will be intepreted as "no piece", which is not valid.

}

void humanMove(board* current, board* next) {

	time_t startTime = time(NULL);
	
	// First up, actually get the move from the human player.
	analysisMove mv;
	
	int i,isValid,command;
	isValid = 0; // shut the compiler up.
	do {
		command = inputMove(&mv);
		
		analysisList myAllowedMoves;
		myAllowedMoves.ix = 0;
		
		switch (command) {
			case COMMAND_PRINTMOVES:
				generateLegalMoveList(current, &myAllowedMoves, 0);
				print("Allowed moves (%u in total)\n", myAllowedMoves.ix);
				printMoveList(&myAllowedMoves);
				print("Current castling rights: ");
				printByte(current->currentCastlingRights);
				print("Pieces moved: ");
				printByte(current->piecesMoved);
				break;
			case COMMAND_UNDO:
				print("Undoing move...\n");
				memcpy((void*)current, (void*)(&undoBoard), sizeof(board));
				printQBUnicode(current->quad);
				break;
			case COMMAND_LOGGING:
				if (logging == 0) {
					openLog();
					print("Turning logging ON\n");
				}
				else {
					print("Turning logging OFF\n");
					closeLog();
				}
				break;
			case COMMAND_MOVE:
				// Check that it's valid.
				generateLegalMoveList(current, &myAllowedMoves, 0);
				isValid = 0;
				for (i = 0; i < myAllowedMoves.ix; i++) {
					
					if (myAllowedMoves.items[i].from == mv.from 
						&& myAllowedMoves.items[i].to == mv.to) {
							
						// Ensure this gets zeroed, we will set it explicilty 
						// iff there's promotion afoot.
						mv.promoteTo = 0;

						if (myAllowedMoves.items[i].promoteTo != 0) {
							
							int promoted = inputPawnPromotion();
							
							if (promoted > 0) {
								
								mv.promoteTo = promoted;
								isValid = 1;
								break; // out of for loop.
							}
						}
						else {
							isValid = 1;
							break; // out of for loop.
						}	
					}
				}
				if (isValid  == 0) {
					print("Entered move is not valid, please try again\n");
				}
				break; // out of while loop.
		}
		
	} while (isValid == 0);


	// Before we actually do anything, ensure we can go back one move.
	memcpy((void*)(&undoBoard), (void*)current, sizeof(board));

	// Now actually make the move
	makeMove(current, next, &mv);

	// Move made, print the result then return to main program loop.
	time_t finishTime = time(NULL);
	double timetaken = difftime(finishTime, startTime);
	
	print("===== human move for ");
	print(current->whosTurn ? "BLACK" : "WHITE");
	printf(" =====\n");
	print("Move chosen: ");
	print(" ");
	printMove(mv);
	if (isKingChecked(next->quad, next->whosTurn)) {
		print(" >>> CHECK <<<");
	}
	printf("\n");
    print("Human Move Time Taken: %f\n", timetaken);

	printQBUnicode(next->quad);
	
}
