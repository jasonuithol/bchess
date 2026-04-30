// ==================================================================
//
// moveordering.c
//
// Move ordering for alpha-beta search optimization
//
// Author:      Jason Uithol
// Copyright:   2016
//
// ==================================================================

// Move ordering scores - higher = search first
#define SCORE_HASH_MOVE         900000
#define SCORE_WINNING_CAPTURE   800000  // Base score, add MVV-LVA
#define SCORE_KILLER_PRIMARY    700000
#define SCORE_KILLER_SECONDARY  600000
#define SCORE_CHECK             500000
#define SCORE_EQUAL_CAPTURE     400000
#define SCORE_LOSING_CAPTURE    300000
#define SCORE_HISTORY_BASE      0       // Add history score

// MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
// Prioritize captures: QxP > RxP > NxP > PxQ > PxR, etc.
static const int victimValue[14] = {
    0,      // 0 - empty
    0,      // 1 - unused
    100,    // 2 - PAWN
    0,      // 3 - unused
    300,    // 4 - ROOK
    0,      // 5 - unused
    290,    // 6 - KNIGHT
    0,      // 7 - unused
    310,    // 8 - BISHOP
    0,      // 9 - unused
    900,    // 10 - QUEEN
    0,      // 11 - unused
    20000,  // 12 - KING (should never be captured, but just in case)
    0       // 13 - unused
};

static const int attackerValue[14] = {
    0,      // 0 - empty
    0,      // 1 - unused
    100,    // 2 - PAWN
    0,      // 3 - unused
    500,    // 4 - ROOK
    0,      // 5 - unused
    300,    // 6 - KNIGHT
    0,      // 7 - unused
    300,    // 8 - BISHOP
    0,      // 9 - unused
    900,    // 10 - QUEEN
    0,      // 11 - unused
    20000,  // 12 - KING
    0       // 13 - unused
};

// Killer moves - store 2 killer moves per ply
#define MAX_KILLER_PLY 64
typedef struct {
    bitboard from;
    bitboard to;
} killerMove;

typedef struct {
    killerMove primary;
    killerMove secondary;
} killerMoves;

static killerMoves killers[MAX_KILLER_PLY];

// History heuristic - [piece][to_square]
// Tracks how often a move causes a cutoff
static int historyTable[16][64];

void clearKillers(void) {
    memset(killers, 0, sizeof(killers));
}

void clearHistory(void) {
    memset(historyTable, 0, sizeof(historyTable));
}

// Add a killer move at this depth
void addKiller(const bitboard from, const bitboard to, const depthType depth) {
    if (depth >= MAX_KILLER_PLY) return;
    
    // Don't add if it's already the primary killer
    if (killers[depth].primary.from == from && killers[depth].primary.to == to) {
        return;
    }
    
    // Shift primary to secondary, add new as primary
    killers[depth].secondary = killers[depth].primary;
    killers[depth].primary.from = from;
    killers[depth].primary.to = to;
}

// Update history score for a move that caused a cutoff
void updateHistory(const byte pieceType, const bitboard to, const depthType depth) {
    offset toSquare = trailingBit_Bitboard(to);
    // Increase score based on depth (deeper = more important)
    historyTable[pieceType][toSquare] += depth * depth;
    
    // Cap at reasonable value to prevent overflow
    if (historyTable[pieceType][toSquare] > 10000) {
        historyTable[pieceType][toSquare] = 10000;
    }
}

// Get history score for a move
int getHistoryScore(const byte pieceType, const bitboard to) {
    offset toSquare = trailingBit_Bitboard(to);
    return historyTable[pieceType][toSquare];
}

// Check if move is a killer
byte isKiller(const bitboard from, const bitboard to, const depthType depth) {
    if (depth >= MAX_KILLER_PLY) return 0;
    
    if (killers[depth].primary.from == from && killers[depth].primary.to == to) {
        return 1; // Primary killer
    }
    if (killers[depth].secondary.from == from && killers[depth].secondary.to == to) {
        return 2; // Secondary killer
    }
    return 0;
}

// Score a move for ordering purposes
int scoreMove(const analysisMove* move, const quadboard* prevBoard, const depthType depth) {

    // Check if this is a capture
    bitboard capturedPiece = getAllPieces(*prevBoard) & move->to;
    
    if (capturedPiece) {
        // This is a capture - score using MVV-LVA
        offset fromSquare = trailingBit_Bitboard(move->from);
        offset toSquare = trailingBit_Bitboard(move->to);
        
        byte attackerType = getType(*prevBoard, fromSquare);
        byte victimType = getType(*prevBoard, toSquare);
        
        // MVV-LVA: (Victim value * 10) - Attacker value
        // This makes QxP score higher than PxQ
        int mvvLva = (victimValue[victimType] * 10) - attackerValue[attackerType];
        
        // Categorize capture
        if (mvvLva > 0) {
            // Winning capture (victim worth more than attacker)
            return SCORE_WINNING_CAPTURE + mvvLva;
        } else if (mvvLva == 0) {
            // Equal capture (RxR, NxN, etc.)
            return SCORE_EQUAL_CAPTURE;
        } else {
            // Losing capture (attacker worth more than victim)
            // Still search these, but lower priority
            return SCORE_LOSING_CAPTURE + mvvLva;
        }
    }
    
    // 3. Killer moves (non-captures that caused cutoffs)
    byte killerType = isKiller(move->from, move->to, depth);
    if (killerType == 1) {
        return SCORE_KILLER_PRIMARY;
    }
    if (killerType == 2) {
        return SCORE_KILLER_SECONDARY;
    }
    
    // 4. Checks - prioritize forcing moves
    if (isKingChecked(move->resultingBoard.quad, move->resultingBoard.whosTurn)) {
        return SCORE_CHECK;
    }
    
    // 5. History heuristic for quiet moves
    offset fromSquare = trailingBit_Bitboard(move->from);
    byte pieceType = getType(*prevBoard, fromSquare);
    int historyScore = getHistoryScore(pieceType, move->to);
    
    return SCORE_HISTORY_BASE + historyScore;
}

// Structure to hold move and its score for sorting
typedef struct {
    analysisMove move;
    int score;
} scoredMove;

// Comparison function for qsort
int compareScoredMoves(const void* a, const void* b) {
    const scoredMove* moveA = (const scoredMove*)a;
    const scoredMove* moveB = (const scoredMove*)b;
    
    // Sort descending (highest score first)
    return moveB->score - moveA->score;
}

// Sort moves by their scores
void sortMoves(analysisList* moveList, const quadboard* prevBoard, const depthType depth) {

    if (moveList->ix <= 1) {
        // No point sorting 0 or 1 moves
        return;
    }

    // Create array of scored moves
    scoredMove scoredMoves[256]; // Max possible moves

    // Score each move
    for (byte i = 0; i < moveList->ix; i++) {
        scoredMoves[i].move = moveList->items[i];
        scoredMoves[i].score = scoreMove(&moveList->items[i], prevBoard, depth);
    }
    
    // Sort using qsort
    qsort(scoredMoves, moveList->ix, sizeof(scoredMove), compareScoredMoves);
    
    // Copy back to move list
    for (byte i = 0; i < moveList->ix; i++) {
        moveList->items[i] = scoredMoves[i].move;
    }
}
