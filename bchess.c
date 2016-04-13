// ================================================================
//
// bchess.c
//
// The main() function is located here.
//

//#define SAFETY_ON
#define SAFETY_OFF


#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h> 
#include "logging.c"
#include "piece.c"
#include "board.c"
#include "moves.c"
#include "ai2.c"
#include "human.c"

int quit(int exitcode) {
    logg("Exiting game with return code %d\n", exitcode);
    closeLog();
    exit(exitcode);
}

int main() {

    // ----------------------
    //
    // EXECUTION STARTS HERE
    //
    // ----------------------

//  openLog();

    time_t startTime = time(NULL);

    // Maintain pointers to a "current" board and a "next move" board.
    board b1,b2;
    board *b1ptr, *b2ptr, *tempPtr;
    b1ptr = &b1;
    b2ptr = &b2;

#ifdef PROFILING_BUILD  
    profilingBoard(b1ptr);
#else
    initBoard(b1ptr);
//  crashTest(b1ptr);
//  pawnPromotionTest(b1ptr);
#endif

    printAllowedMoves(b1ptr);
    printBoardUnicode(b1ptr);
//  exit(1);

    print("\n-------------- ai test ----------------\n");

    int turn;
#ifdef PROFILING_BUILD  
    for (turn = 0; turn < 2; turn++) { // for this demo, limit to 50 moves.
#else       
    for (turn = 0; turn <= 200; turn++) { // for this demo, limit to 50 moves.
#endif      
        print("==== TURN %d =====\n\n",turn);

        // For this demo, the AI plays black.
        if (b1ptr->whosTurn == WHITE) {
            aiMove(b1ptr,b2ptr, turn);
//          humanMove(b1ptr,b2ptr);
        }
        else {
            aiMove(b1ptr,b2ptr, turn);
        }

        // swap board pointers to make the new board the next old board.
        tempPtr = b1ptr;
        b1ptr = b2ptr;
        b2ptr = tempPtr;
        
        int gamestate = detectCheckmate(b1ptr);
        switch(gamestate) {
            case BOARD_CHECKMATE:
                print("CHECKMATE !!! Victory to ");
                printTeam(teamOf(opponentOf(b1ptr->whosTurn)) );
                print("\n");
                quit(0);    
            case BOARD_STALEMATE:
                print("STALEMATE !!! It's a draw.\n");
                quit(0);    
        }
        
    }

    time_t finishTime = time(NULL);
    printf("Overall Time Taken: %f\n", difftime(finishTime, startTime));
    
    quit(0);
}
