// ==================================================================
//
// uci.c
//
// Universal Chess Interface protocol implementation
//
// Author:      Jason Uithol
// Copyright:   2016
//
// ==================================================================

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "uci.h"
#include "ai2.h"
#include "aileaf.h"
#include "bitboard.h"
#include "moveordering.h"
#include "quadboard.h"
#include "tt.h"
#include "umpire.h"

// Convert bitboard square to UCI notation (e.g., e2, e4)
static void squareToUCI(bitboard square, char* output) {
    offset sq = trailingBit_Bitboard(square);
    int file = 7 - (sq % 8);  // 0-7, where 0 is 'h' and 7 is 'a'
    int rank = sq / 8;         // 0-7, where 0 is rank 1
    
    output[0] = 'a' + file;
    output[1] = '1' + rank;
    output[2] = '\0';
}

// Convert UCI notation to bitboard square
static bitboard uciToSquare(const char* uci) {
    int file = uci[0] - 'a';  // 0-7
    int rank = uci[1] - '1';  // 0-7
    
    offset sq = rank * 8 + (7 - file);
    return 1ULL << sq;
}

// Print a move in UCI format (e.g., "e2e4" or "e7e8q" for promotion)
static void printMoveUCI(const analysisMove* move, const quadboard* board) {
    char from[3], to[3];
    squareToUCI(move->from, from);
    squareToUCI(move->to, to);
    
    // Check for pawn promotion
    offset fromSquare = trailingBit_Bitboard(move->from);
    byte pieceType = getType(*board, fromSquare);
    
    if (pieceType == PAWN) {
        int toRank = trailingBit_Bitboard(move->to) / 8;
        if (toRank == 0 || toRank == 7) {
            // Promotion - UCI format: e7e8q
            // For now, always promote to queen
            printf("%s%sq", from, to);
            return;
        }
    }
    
    printf("%s%s", from, to);
}

// Main UCI loop
void uciLoop(void) {
    char line[4096];
    board currentBoard;
    initBoard(&currentBoard);

    // For move history (to detect loops)
    board history[5];
    for (int i = 0; i < 5; i++) {
        history[i] = currentBoard;
    }
    int historyIndex = 0;

    // Verbose debug output is off by default. Toggle with
    //   setoption name Debug value true
    int debugMode = 0;

    printf("# bchess UCI engine\n");
    fflush(stdout);
    
    while (fgets(line, sizeof(line), stdin)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // UCI command: "uci"
        if (strcmp(line, "uci") == 0) {
            printf("id name bchess\n");
            printf("id author Jason Uithol\n");
            printf("option name Debug type check default false\n");
            printf("uciok\n");
            fflush(stdout);
        }
        // UCI command: setoption name Debug value true|false
        else if (strncmp(line, "setoption", 9) == 0) {
            if (strstr(line, "name Debug")) {
                debugMode = (strstr(line, "value true") != NULL);
            }
        }
        
        // UCI command: "isready"
        else if (strcmp(line, "isready") == 0) {
            printf("readyok\n");
            fflush(stdout);
        }
        
        // UCI command: "ucinewgame"
        else if (strcmp(line, "ucinewgame") == 0) {
            initBoard(&currentBoard);
            clearKillers();
            clearHistory();
            historyIndex = 0;
            for (int i = 0; i < 5; i++) {
                history[i] = currentBoard;
            }
        }
        
        // UCI command: "position startpos" or "position startpos moves e2e4 e7e5..."
        else if (strncmp(line, "position startpos", 17) == 0) {
            initBoard(&currentBoard);
            historyIndex = 0;

            if (debugMode) {
                analysisList moveList;
                moveList.ix = 0;
                generateLegalMoveList(&currentBoard, &moveList, 0);
                printf("info string startpos: %d legal moves, castling=%d\n",
                       moveList.ix, currentBoard.currentCastlingRights);
                fflush(stdout);
            }

            // Apply trailing "moves ..." sequence if present.
            char* movesPtr = strstr(line, "moves");
            if (movesPtr) {
                movesPtr += 6;

                char* token = strtok(movesPtr, " ");
                while (token != NULL) {
                    if (strlen(token) >= 4) {
                        bitboard from = uciToSquare(token);
                        bitboard to   = uciToSquare(token + 2);

                        analysisList moveList;
                        moveList.ix = 0;
                        generateLegalMoveList(&currentBoard, &moveList, 0);

                        byte found = 0;
                        for (byte i = 0; i < moveList.ix; i++) {
                            if (moveList.items[i].from == from && moveList.items[i].to == to) {
                                history[historyIndex % 5] = currentBoard;
                                historyIndex++;

                                board nextBoard;
                                spawnFullBoard(&currentBoard, &nextBoard,
                                               moveList.items[i].from,
                                               moveList.items[i].to,
                                               moveList.items[i].promoteTo);
                                currentBoard = nextBoard;
                                found = 1;
                                break;
                            }
                        }

                        // Always surface a rejected move. If we silently
                        // skip it, side-to-move desyncs from the GUI and
                        // every subsequent "go" generates moves for the
                        // wrong colour.
                        if (!found) {
                            printf("info string ERROR: move %s not found in legal list\n", token);
                            fflush(stdout);
                        }
                    }
                    token = strtok(NULL, " ");
                }
            }
        }
        
        // UCI command: "go"
        else if (strncmp(line, "go", 2) == 0) {

            // Parse parameters. Recognized: depth, wtime, btime, winc, binc,
            // movetime, movestogo. Unknown tokens are skipped.
            int depthLimit = 64;
            int wtime = 0, btime = 0, winc = 0, binc = 0;
            int movetime = 0, movestogo = 30;
            int gotTimeBudget = 0;

            // strtok mutates the buffer; we're done with it after this branch.
            char* tok = strtok(line, " ");
            while (tok) {
                if (strcmp(tok, "depth") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) depthLimit = atoi(tok);
                } else if (strcmp(tok, "wtime") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) { wtime = atoi(tok); gotTimeBudget = 1; }
                } else if (strcmp(tok, "btime") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) { btime = atoi(tok); gotTimeBudget = 1; }
                } else if (strcmp(tok, "winc") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) winc = atoi(tok);
                } else if (strcmp(tok, "binc") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) binc = atoi(tok);
                } else if (strcmp(tok, "movetime") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) { movetime = atoi(tok); gotTimeBudget = 1; }
                } else if (strcmp(tok, "movestogo") == 0) {
                    tok = strtok(NULL, " ");
                    if (tok) movestogo = atoi(tok);
                }
                tok = strtok(NULL, " ");
            }

            // Compute the per-move time budget (in ms). If neither movetime
            // nor wtime/btime was given, only the depth limit applies.
            long budgetMs = -1;
            if (movetime > 0) {
                budgetMs = movetime;
            } else if (gotTimeBudget) {
                int myTime = currentBoard.whosTurn == WHITE ? wtime : btime;
                int myInc  = currentBoard.whosTurn == WHITE ? winc  : binc;
                if (movestogo <= 0) movestogo = 30;
                budgetMs = myTime / movestogo + myInc / 2;
            }
            // Reserve a small safety margin so we don't lose on time.
            if (budgetMs > 50) {
                budgetMs -= 20;
            } else if (budgetMs > 0) {
                budgetMs /= 2;
            }

            // Search for best move
            nodesCalculated = 0;
            ttInit();
            ttNewSearch();
            analysisMove bestMove;

            // Get loop detection board
            board* loopDetect = &history[(historyIndex - 4 + 5) % 5];

            // Iterative deepening with optional time bail-out. Don't start
            // a new iteration if we've already used 40% of the budget —
            // each iteration typically takes 3-5x its predecessor, so
            // pushing further usually overruns.
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            scoreType score = 0;
            int finalDepth = 1;
            for (depthType d = 1; d <= depthLimit; d++) {
                score = getBestMove(
                    &bestMove,
                    loopDetect,
                    &currentBoard,
                    currentBoard.whosTurn,
                    d,
                    0,
                    -9999,
                    9999
                );
                clock_gettime(CLOCK_MONOTONIC, &t1);
                long elapsedMs = (t1.tv_sec - t0.tv_sec) * 1000L
                               + (t1.tv_nsec - t0.tv_nsec) / 1000000L;
                long nps = elapsedMs > 0 ? ((long)nodesCalculated * 1000L) / elapsedMs : 0;
                printf("info depth %d score cp %d nodes %u nps %ld time %ld\n",
                       d, (int)score, (unsigned int)nodesCalculated, nps, elapsedMs);
                fflush(stdout);
                finalDepth = d;
                if (budgetMs > 0 && elapsedMs * 5 >= budgetMs * 2) {
                    break;
                }
            }
            (void)finalDepth;

            // Output best move
            printf("bestmove ");
            printMoveUCI(&bestMove, &currentBoard.quad);
            printf("\n");
            fflush(stdout);
        }
        
        // UCI command: "quit"
        else if (strcmp(line, "quit") == 0) {
            break;
        }
        
        // DEBUG command: "d" - display board and legal moves
        else if (strcmp(line, "d") == 0) {
            printf("# Current position:\n");
            printQB(currentBoard.quad);
            printf("#\n");
            printf("# Legal moves:\n");
            
            analysisList moveList;
            moveList.ix = 0;
            generateLegalMoveList(&currentBoard, &moveList, 0);
            
            for (byte i = 0; i < moveList.ix; i++) {
                printf("# ");
                printMoveUCI(&moveList.items[i], &currentBoard.quad);
                printf("\n");
            }
            printf("# Total moves: %d\n", moveList.ix);
            fflush(stdout);
        }
        
        // DEBUG command: "perft X" - count moves at depth X
        else if (strncmp(line, "perft", 5) == 0) {
            int depth = 1;
            sscanf(line, "perft %d", &depth);
            
            // Simple perft - just count legal moves at depth
            printf("# Running perft to depth %d...\n", depth);
            // TODO: implement recursive perft if needed
            fflush(stdout);
        }
    }
}
