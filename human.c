//
// Ask a human agent to make a move.
//

void inputMove(move* parsedMove) {
	
   char buffer[20];
	
   fputs("enter your move (e.g. a1-b2): ", stdout);
   fflush(stdout);
   if (fgets(buffer, sizeof(buffer), stdin) != NULL) {

	   parsedMove->from.x = (byte)(buffer[0] - 'a');
	   parsedMove->from.y = (byte)(buffer[1] - '1');
	   parsedMove->to.x = (byte)(buffer[3] - 'a');
	   parsedMove->to.y = (byte)(buffer[4] - '1');
	   
//      char *newline = strchr(buffer, '\n'); /* search for newline character */
//      if (newline != NULL) {
//         *newline = '\0'; /* overwrite trailing newline */
//      }
   }
   else {
	   printf("fgets() is no better than anything else around\n");
	   fflush(stdout);
   }
   
   	
}


void humanMove(board* current, board* next) {

	time_t startTime = time(NULL);
	
	// First up, actually get the move from the human player.
	move mv;
	
	int i,isValid;
	do {
		inputMove(&mv);
		
		// Check that it's valid.
		moveList myAllowedMoves;
		allowedMoves(&myAllowedMoves, current, current->whosTurn);
		isValid = 0;
		for (i = 0; i < myAllowedMoves.ix; i++) {
			if (myAllowedMoves.moves[i].from.x == mv.from.x 
				&& myAllowedMoves.moves[i].from.y == mv.from.y
				&& myAllowedMoves.moves[i].to.x == mv.to.x
				&& myAllowedMoves.moves[i].to.y == mv.to.y) {
					
					isValid = 1;
//					break; // breaks to while loop, not main function.
				}
		}
		if (isValid  == 0) {
			printf("Entered move is not valid, please try again\n");
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
