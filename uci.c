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

// Convert bitboard square to UCI notation (e.g., e2, e4)
void squareToUCI(bitboard square, char* output) {
    offset sq = trailingBit_Bitboard(square);
    int file = 7 - (sq % 8);  // 0-7, where 0 is 'h' and 7 is 'a'
    int rank = sq / 8;         // 0-7, where 0 is rank 1
    
    output[0] = 'a' + file;
    output[1] = '1' + rank;
    output[2] = '\0';
}

// Convert UCI notation to bitboard square
bitboard uciToSquare(const char* uci) {
    int file = uci[0] - 'a';  // 0-7
    int rank = uci[1] - '1';  // 0-7
    
    offset sq = rank * 8 + (7 - file);
    return 1ULL << sq;
}

// Print a move in UCI format (e.g., "e2e4" or "e7e8q" for promotion)
void printMoveUCI(const analysisMove* move, const quadboard* board) {
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
    
    printf("# bchess UCI engine\n");
    fflush(stdout);
    
    while (fgets(line, sizeof(line), stdin)) {
        // Remove newline
        line[strcspn(line, "\r\n")] = 0;
        
        // UCI command: "uci"
        if (strcmp(line, "uci") == 0) {
            printf("id name bchess\n");
            printf("id author Jason Uithol\n");
            // Options can go here
            printf("uciok\n");
            fflush(stdout);
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
            
            // Check for moves
            char* movesPtr = strstr(line, "moves");
            if (movesPtr) {
                movesPtr += 6; // Skip "moves "
                
                // Parse and apply each move
                char* token = strtok(movesPtr, " ");
                while (token != NULL) {
                    // Parse move (e.g., "e2e4")
                    if (strlen(token) >= 4) {
                        bitboard from = uciToSquare(token);
                        bitboard to = uciToSquare(token + 2);
                        
                        // Find and apply this move
                        analysisList moveList;
                        moveList.ix = 0;
                        generateLegalMoveList(&currentBoard, &moveList, 0);
                        
                        for (byte i = 0; i < moveList.ix; i++) {
                            if (moveList.items[i].from == from && moveList.items[i].to == to) {
                                // Store in history
                                history[historyIndex % 5] = currentBoard;
                                historyIndex++;
                                
                                // Apply move
                                currentBoard = moveList.items[i].resultingBoard;
                                break;
                            }
                        }
                    }
                    token = strtok(NULL, " ");
                }
            }
        }
        
        // UCI command: "go"
        else if (strncmp(line, "go", 2) == 0) {
            // Parse go parameters (depth, time, etc.)
            // For now, use fixed depth
            depthType depth = 4;  // Default depth
            
            // Look for "depth X" in the command
            char* depthPtr = strstr(line, "depth");
            if (depthPtr) {
                sscanf(depthPtr, "depth %hhu", &depth);
            }
            
            // Search for best move
            nodesCalculated = 0;
            analysisMove bestMove;
            
            // Get loop detection board
            board* loopDetect = &history[(historyIndex - 4 + 5) % 5];
            
            scoreType score = getBestMove(
                &bestMove, 
                loopDetect,
                &currentBoard, 
                currentBoard.whosTurn, 
                depth, 
                0, 
                -9999, 
                9999, 
                0, 
                0
            );
            
            // Output info
            printf("info depth %d score cp %d nodes %u\n", 
                   depth, (int)score, (unsigned int)nodesCalculated);
            
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
    }
}
