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
#include "attacks.h"
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

// Recursively count leaf nodes at depth N from board b. Standard chess
// "perft" benchmark, used to validate move generation: a regression
// will almost always show up as a count mismatch.
static uint64_t perftRec(board* b, int depth) {
    if (depth == 0) return 1;

    analysisList moveList;
    moveList.ix = 0;
    generateLegalMoveList(b, &moveList, 0);

    if (depth == 1) {
        // Cheap leaf: each legal move contributes exactly 1.
        return (uint64_t)moveList.ix;
    }

    uint64_t nodes = 0;
    for (byte i = 0; i < moveList.ix; i++) {
        board next;
        spawnFullBoard(b, &next,
                       moveList.items[i].from,
                       moveList.items[i].to,
                       moveList.items[i].promoteTo);
        nodes += perftRec(&next, depth - 1);
    }
    return nodes;
}

// Parse a FEN string into the supplied board. Returns 1 on success, 0 on
// malformed input. bchess doesn't model en-passant or halfmove counters,
// so those FEN fields are consumed and discarded.
static int parseFen(const char* fen, board* b) {
    clearBoard(b);

    int sq_rank = 7;
    int sq_file = 0;
    const char* p = fen;

    // Field 1: piece placement.
    while (*p && *p != ' ') {
        char c = *p++;
        if (c == '/') {
            sq_rank--;
            sq_file = 0;
            continue;
        }
        if (c >= '1' && c <= '8') {
            sq_file += (c - '0');
            continue;
        }
        if (sq_rank < 0 || sq_rank > 7 || sq_file < 0 || sq_file > 7) return 0;

        byte team = (c >= 'a' && c <= 'z') ? BLACK : WHITE;
        byte type = 0;
        switch (c | 0x20) {  // tolower
            case 'p': type = PAWN;   break;
            case 'n': type = KNIGHT; break;
            case 'b': type = BISHOP; break;
            case 'r': type = ROOK;   break;
            case 'q': type = QUEEN;  break;
            case 'k': type = KING;   break;
            default: return 0;
        }
        offset sq = sq_rank * 8 + (7 - sq_file);
        addPieces(&b->quad, 1ULL << sq, type | team);
        sq_file++;
    }

    if (*p == ' ') p++;

    // Field 2: side to move.
    if (*p == 'w') b->whosTurn = WHITE;
    else if (*p == 'b') b->whosTurn = BLACK;
    else return 0;
    p++;
    if (*p == ' ') p++;

    // Field 3: castling rights. Translate to bchess's piecesMoved bitset
    // (every right ABSENT means the corresponding piece "has moved").
    b->piecesMoved = WHITE_KING_MOVED | WHITE_KINGSIDE_CASTLE_MOVED
                   | WHITE_QUEENSIDE_CASTLE_MOVED
                   | BLACK_KING_MOVED | BLACK_KINGSIDE_CASTLE_MOVED
                   | BLACK_QUEENSIDE_CASTLE_MOVED;
    if (*p == '-') {
        p++;
    } else {
        while (*p && *p != ' ') {
            switch (*p) {
                case 'K': b->piecesMoved &= ~(WHITE_KING_MOVED | WHITE_KINGSIDE_CASTLE_MOVED); break;
                case 'Q': b->piecesMoved &= ~(WHITE_KING_MOVED | WHITE_QUEENSIDE_CASTLE_MOVED); break;
                case 'k': b->piecesMoved &= ~(BLACK_KING_MOVED | BLACK_KINGSIDE_CASTLE_MOVED); break;
                case 'q': b->piecesMoved &= ~(BLACK_KING_MOVED | BLACK_QUEENSIDE_CASTLE_MOVED); break;
                default: return 0;
            }
            p++;
        }
    }

    // Remaining fields (en-passant target, halfmove, fullmove) are
    // consumed but ignored.
    computeCurrentCastlingRights(b);
    return 1;
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

    // Spinner output is for humans watching a TUI. In UCI mode it gets
    // wedged into the "bestmove" line and breaks GUIs/match runners.
    suppressSpinnerOutput = 1;

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
        
        // UCI command: "position fen <fen> [moves ...]"
        else if (strncmp(line, "position fen ", 13) == 0) {
            // The FEN is everything from after "position fen " up to but
            // not including the optional " moves ..." suffix.
            char* movesPtr = strstr(line + 13, "moves");
            if (movesPtr) {
                // Temporarily nul-terminate the FEN.
                char saved = *movesPtr;
                *movesPtr = '\0';
                if (!parseFen(line + 13, &currentBoard)) {
                    printf("info string ERROR: failed to parse FEN\n");
                    fflush(stdout);
                }
                *movesPtr = saved;
            } else {
                if (!parseFen(line + 13, &currentBoard)) {
                    printf("info string ERROR: failed to parse FEN\n");
                    fflush(stdout);
                }
            }
            historyIndex = 0;

            // Apply the same trailing "moves ..." suffix logic that the
            // startpos branch uses.
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

                        if (!found) {
                            printf("info string ERROR: move %s not found in legal list\n", token);
                            fflush(stdout);
                        }
                    }
                    token = strtok(NULL, " ");
                }
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
            analysisMove prevBestMove;
            scoreType prevScore = 0;
            int havePrev = 0;

            // Get loop detection board
            board* loopDetect = &history[(historyIndex - 4 + 5) % 5];

            // Iterative deepening with optional time bail-out. Don't start
            // a new iteration if we've already used 40% of the budget —
            // each iteration typically takes 3-5x its predecessor, so
            // pushing further usually overruns. The deadline below is a
            // safety net for the case where an iteration that did start
            // explodes mid-search.
            struct timespec t0, t1;
            clock_gettime(CLOCK_MONOTONIC, &t0);
            const long t0Ms = t0.tv_sec * 1000L + t0.tv_nsec / 1000000L;

            // Iteration 1 always runs to completion (cheap, and we need
            // *some* legal move to emit even on a pathologically tight
            // budget). The hard deadline is armed only after we have a
            // completed iteration to fall back on.
            clearSearchDeadline();

            scoreType score = 0;
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

                if (searchAborted) {
                    // Iteration d>=2 exceeded the budget. bestMove is
                    // partial garbage; fall back to the last completed
                    // iteration's result.
                    if (havePrev) {
                        bestMove = prevBestMove;
                        score    = prevScore;
                    }
                    break;
                }

                prevBestMove = bestMove;
                prevScore    = score;
                havePrev     = 1;

                clock_gettime(CLOCK_MONOTONIC, &t1);
                long elapsedMs = (t1.tv_sec - t0.tv_sec) * 1000L
                               + (t1.tv_nsec - t0.tv_nsec) / 1000000L;
                long nps = elapsedMs > 0 ? ((long)nodesCalculated * 1000L) / elapsedMs : 0;
                printf("info depth %d score cp %d nodes %u nps %ld time %ld\n",
                       d, (int)score, (unsigned int)nodesCalculated, nps, elapsedMs);
                fflush(stdout);

                if (budgetMs > 0 && elapsedMs * 5 >= budgetMs * 2) {
                    break;
                }

                // Arm the deadline once we have a result to fall back on.
                if (budgetMs > 0) {
                    setSearchDeadlineMs(t0Ms + budgetMs);
                }
            }

            clearSearchDeadline();

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
            // printQB is broken (first newline fires before any rank
            // completes), so draw the board ourselves with rank/file
            // labels and standard piece letters.
            printf("# Current position (%s to move):\n",
                   currentBoard.whosTurn == WHITE ? "white" : "black");
            for (int rank = 7; rank >= 0; rank--) {
                printf("# %d ", rank + 1);
                for (int file = 0; file < 8; file++) {
                    offset sq = rank * 8 + (7 - file);
                    byte type = getType(currentBoard.quad, sq);
                    char c = '.';
                    switch (type) {
                        case PAWN:   c = 'P'; break;
                        case ROOK:   c = 'R'; break;
                        case KNIGHT: c = 'N'; break;
                        case BISHOP: c = 'B'; break;
                        case QUEEN:  c = 'Q'; break;
                        case KING:   c = 'K'; break;
                    }
                    if (type && getTeam(currentBoard.quad, sq)) {
                        c = c - 'A' + 'a';
                    }
                    printf("%c ", c);
                }
                printf("\n");
            }
            printf("#   a b c d e f g h\n");
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
        
        // DEBUG command: "perft N" — count legal leaf nodes at depth N.
        // Output mirrors the de-facto convention: per-move counts, blank
        // line, then "Nodes searched: N".
        else if (strncmp(line, "perft", 5) == 0) {
            int depth = 1;
            if (sscanf(line, "perft %d", &depth) != 1 || depth < 0) depth = 1;

            analysisList moveList;
            moveList.ix = 0;
            generateLegalMoveList(&currentBoard, &moveList, 0);

            uint64_t total = 0;
            for (byte i = 0; i < moveList.ix; i++) {
                board next;
                spawnFullBoard(&currentBoard, &next,
                               moveList.items[i].from,
                               moveList.items[i].to,
                               moveList.items[i].promoteTo);
                uint64_t sub = (depth <= 1) ? 1 : perftRec(&next, depth - 1);
                total += sub;

                printMoveUCI(&moveList.items[i], &currentBoard.quad);
                printf(": %llu\n", (unsigned long long)sub);
            }
            printf("\nNodes searched: %llu\n", (unsigned long long)total);
            fflush(stdout);
        }

        // DEBUG command: "bench" — run a fixed search at depth across a
        // hardcoded corpus and emit a single summary "Nodes: N" line.
        // The corpus is intentionally small so the whole run stays under
        // a few seconds, and intentionally fixed so any regression in
        // search behaviour shows up as a different node count.
        else if (strcmp(line, "bench") == 0) {
            static const char* const benchPositions[] = {
                // Mid-opening, mid-middlegame, simplification, late
                // endgame — covers the main eval regimes.
                "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1",
                "r1bqkb1r/pppp1ppp/2n2n2/4p3/2B1P3/5N2/PPPP1PPP/RNBQK2R w KQkq - 4 4",
                "r2qk2r/ppp2ppp/2nb1n2/3pp3/3PP3/2NB1N2/PPP2PPP/R2QK2R w KQkq - 0 7",
                "8/8/4k3/8/8/2K5/4P3/8 w - - 0 1",
                "r3k2r/pp1q1ppp/2nbpn2/3p4/3P4/2NBPN2/PP1Q1PPP/R3K2R w KQkq - 0 9",
            };
            const int nPositions = (int)(sizeof(benchPositions) / sizeof(benchPositions[0]));
            const int benchDepth = 6;

            uint64_t totalNodes = 0;
            board savedBoard = currentBoard;

            for (int p = 0; p < nPositions; p++) {
                if (!parseFen(benchPositions[p], &currentBoard)) {
                    printf("# bench: failed to parse position %d\n", p);
                    continue;
                }

                nodesCalculated = 0;
                ttInit();
                ttNewSearch();
                clearSearchDeadline();

                analysisMove bm;
                scoreType score = 0;
                for (depthType d = 1; d <= benchDepth; d++) {
                    score = getBestMove(&bm, &currentBoard, &currentBoard,
                                        currentBoard.whosTurn, d, 0, -9999, 9999);
                }
                totalNodes += nodesCalculated;

                printf("# bench[%d]: depth=%d score=%d nodes=%u bestmove=",
                       p, benchDepth, (int)score, (unsigned)nodesCalculated);
                printMoveUCI(&bm, &currentBoard.quad);
                printf("\n");
            }

            currentBoard = savedBoard;
            printf("\nNodes: %llu\n", (unsigned long long)totalNodes);
            fflush(stdout);
        }
    }
}
