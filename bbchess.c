#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Windows UTF-8 support
#ifdef _WIN32
#include <windows.h>
#endif

#include "logging.h"
#include "bitboard.h"
#include "iterator.h"
#include "quadboard.h"
#include "attacks.h"
#include "umpire.h"
#include "savegame.h"
#include "aileaf.h"
#include "moveordering.h"
#include "ai2.h"
#include "airoot.h"
#include "human.h"

#ifdef BUILD_TESTING
#include "test.h"
#include "performance.h"

void diagnostics(void) {
    board b;
    initBoard(&b);
    
    
    printf("8ULL << 7 = %lld\n", 8ULL << 7);
    bitboard testb = 8;
    offset testo = 7;
    bitboard result = testb << testo;
    printf("testb << test0 = %" PRId64 "\n", result);
    printBB(result);
    
    print("WHITE King, Friends and Enemies\n");
    printBB(getPieces(b.quad , WHITE | KING));   printf("\n");
    printBB(getTeamPieces(b.quad, WHITE)); printf("\n");
    printBB(getTeamPieces(b.quad, BLACK)); printf("\n");
    
    print("Showing the whole board\n\n");
    printQB(b.quad);
    print("Showing board components. Type0\n");
    printBB(b.quad.type0);
    print("Type1\n");
    printBB(b.quad.type1);
    print("Type2\n");
    printBB(b.quad.type2);
    print("Team\n");
    printBB(b.quad.team);
    testSuite();    
    runPerformanceSuite();
}

int main(void) {
    diagnostics();
    return 0;
}

#else

void run(gameContext* game) {
    time_t startTime = time(NULL);
    board* b1ptr = &(game->boards[game->current]);
    board* b2ptr = &(game->boards[game->next]);
    board* loopDetectPtr = &(game->boards[game->loopDetectIx]);

//    printQBUnicode(b1ptr->quad);
    printQB(b1ptr->quad);
    
#ifdef BUILD_BENCHMARK
    while (game->turn <= 20) { 
#else
    while (game->turn <= 200) { 
#endif
        print("==== TURN %d =====\n\n",game->turn);
        
//      printBB(getTeamPieces(b1ptr->quad, b1ptr->whosTurn));
        
        print("Current board score: %d\n", (int)analyseLeafNonTerminal(b1ptr->quad,b1ptr->whosTurn));
        if (b1ptr->whosTurn == WHITE) {
#ifdef WHITE_HUMAN
            humanMove(b1ptr,b2ptr);
#else
            aiMove(b1ptr,b2ptr,loopDetectPtr,game->turn);
#endif
        }
        else {
#ifdef BLACK_HUMAN
            humanMove(b1ptr,b2ptr);
#else           
            aiMove(b1ptr,b2ptr,loopDetectPtr,game->turn);
#endif
        }
        
        int gamestate = detectCheckmate(b2ptr);
        
        switch(gamestate) {
            case BOARD_CHECKMATE:
                print("CHECKMATE !!! Victory to ");
                print(b1ptr->whosTurn ? "BLACK" : "WHITE");
                print("\n");
                exit(0);
            case BOARD_STALEMATE:
                print("STALEMATE !!! It's a draw.\n");
                exit(0);
        }
        if (areEqualQB(b2ptr->quad, loopDetectPtr->quad)) {
            game->loopCount++;
            
            if (game->loopCount >= 3) {
                print("DRAW BY 3 LOOPS !!!\n");
                exit(0);
            }
        }
        //
        // Increment the board indexes, and update the respective pointers.
        //
        game->current = (game->current + 1) % 5;
        game->next = (game->next + 1) % 5;
        game->loopDetectIx = (game->loopDetectIx + 1) % 5;
        b1ptr = &(game->boards[game->current]);
        b2ptr = &(game->boards[game->next]);
        loopDetectPtr = &(game->boards[game->loopDetectIx]);
        game->turn++;
        
        save(game);             
    }
    time_t finishTime = time(NULL);
    printf("Overall Time Taken: %f\n", difftime(finishTime, startTime));
}

int main(void) {
    // ----------------------
    //
    // EXECUTION STARTS HERE
    //
    // ----------------------
    
#ifdef _WIN32

    // Enable ANSI escape sequences (for colors) FIRST
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD dwMode = 0;
        if (GetConsoleMode(hOut, &dwMode)) {
            dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
            SetConsoleMode(hOut, dwMode);
        }
    }
    
    // THEN set UTF-8 (order matters!)
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Force the console font to something that supports Unicode
    // This is a workaround for cmd.exe font rendering bugs
    system("chcp 65001 >nul");

#endif
    
    gameContext game;
    load(&game);
    
    run(&game);
    
    return 0;
}

#endif
