//
// Ask a human agent to make a move.
//

#define COMMAND_MOVE (0)
#define COMMAND_PRINTMOVES (1)
#define COMMAND_UNDO (2)
#define COMMAND_LOGGING (3)

board undoBoard;

int inputMove(move* parsedMove) {
	
   char buffer[20];
	
   print("enter your move (e.g. a1-b2): ", stdout);
   if (fgets(buffer, sizeof(buffer), stdin) != NULL) {

		switch (buffer[0]) {
			case 'M': return COMMAND_PRINTMOVES; break;
			case 'U': return COMMAND_UNDO; break;
			case 'L': return COMMAND_LOGGING; break;
		}

		parsedMove->from.x = (byte)(buffer[0] - 'a');
		parsedMove->from.y = (byte)(buffer[1] - '1');
		parsedMove->to.x = (byte)(buffer[3] - 'a');
		parsedMove->to.y = (byte)(buffer[4] - '1');
	   
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
	move mv;
	
	int i,isValid,command;
	do {
		command = inputMove(&mv);
		
		moveList myAllowedMoves;
		
		switch (command) {
			case COMMAND_PRINTMOVES:
				printAllowedMoves(current);
				break;
			case COMMAND_UNDO:
				print("Undoing move...\n");
				memcpy((void*)current, (void*)(&undoBoard), sizeof(board));
				printBoardUnicode(current);
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
				allowedMoves(&myAllowedMoves, current, current->whosTurn);
				isValid = 0;
				for (i = 0; i < myAllowedMoves.ix; i++) {
					if (myAllowedMoves.moves[i].from.x == mv.from.x 
						&& myAllowedMoves.moves[i].from.y == mv.from.y
						&& myAllowedMoves.moves[i].to.x == mv.to.x
						&& myAllowedMoves.moves[i].to.y == mv.to.y) {
							
						// Ensure this gets zeroed, we will set it explicilty 
						// iff there's promotion afoot.
						mv.promoteTo = 0;

						if (myAllowedMoves.moves[i].promoteTo != 0) {
							
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
	makeMove(current, next, mv);

	// Move made, print the result then return to main program loop.
	time_t finishTime = time(NULL);
	double timetaken = difftime(finishTime, startTime);
	
	print("===== human move for ");printTeam(current->whosTurn);printf(" =====\n");
	print("Move chosen: ");
	print(" ");printMove(current, mv);printf("\n");
    print("Human Move Time Taken: %f\n", timetaken);

	printBoardUnicode(next);
	
}
