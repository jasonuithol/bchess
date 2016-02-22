//
// Ask a human agent to make a move.
//

#define COMMAND_MOVE (0)
#define COMMAND_PRINTMOVES (1)

int inputMove(move* parsedMove) {
	
   char buffer[20];
	
   fputs("enter your move (e.g. a1-b2): ", stdout);
   fflush(stdout);
   if (fgets(buffer, sizeof(buffer), stdin) != NULL) {

		switch (buffer[0]) {
			case 'M': return COMMAND_PRINTMOVES;
		}

		parsedMove->from.x = (byte)(buffer[0] - 'a');
		parsedMove->from.y = (byte)(buffer[1] - '1');
		parsedMove->to.x = (byte)(buffer[3] - 'a');
		parsedMove->to.y = (byte)(buffer[4] - '1');
	   
   }
   else {
	   printf("fgets() is no better than anything else around\n");
	   fflush(stdout);
   }
   
   return COMMAND_MOVE;	
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
			case COMMAND_MOVE:
				// Check that it's valid.
				allowedMoves(&myAllowedMoves, current, current->whosTurn);
				isValid = 0;
				for (i = 0; i < myAllowedMoves.ix; i++) {
					if (myAllowedMoves.moves[i].from.x == mv.from.x 
						&& myAllowedMoves.moves[i].from.y == mv.from.y
						&& myAllowedMoves.moves[i].to.x == mv.to.x
						&& myAllowedMoves.moves[i].to.y == mv.to.y) {
							
							isValid = 1;
					}
				}
				if (isValid  == 0) {
					printf("Entered move is not valid, please try again\n");
				}
				break;
		}
		
	} while (isValid == 0);

	// Now actually make the move
	makeMove(current, next, mv);

	// Move made, print the result then return to main program loop.
	time_t finishTime = time(NULL);
	double timetaken = difftime(finishTime, startTime);
	
	printf("===== human move for ");printTeam(current->whosTurn);printf(" =====\n");
	printf("Move chosen: ");
	printf(" ");printMove(current, mv);printf("\n");
    printf("Human Move Time Taken: %f\n", timetaken);

	printBoardClassic(next);
	
	fflush(stdout);	
}
