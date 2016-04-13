#define SAVEFILE ("bchess.save")

typedef struct {

    int turn;

    board boards[5];
    
    byte current;
    byte next;
    byte loopDetectIx;
    byte loopCount;
    
} gameContext;

void newGame(gameContext* game) {

    game->turn = 0;

    game->current = 0;
    game->next = 1;
    
    game->loopDetectIx = 2;
    game->loopCount = 0;

    for (byte i = 0; i < 5; i++) {
        clearBoard(&(game->boards[i]));
    }

    initBoard(&(game->boards[game->current]));  
}

void load(gameContext* game) {

    FILE* f = fopen(SAVEFILE, "r+");
    
    if (f == NULL) {
        print("No save game file, creating a new game.\n");
        newGame(game);
    }
    else {
        
        if (fread(game, sizeof(gameContext), 1, f) == -1) {
            print("Unable to load existing saved game, creating a new game.\n");
            newGame(game);
        }
        else {
            print("Loading existing game.\n");
        }
        
        fclose(f); // Does a free(f) for you.
    }   
}

void save(gameContext* game) {

    FILE* f = fopen(SAVEFILE, "w");
    
    if (f == NULL) {
        print("Unable to open save file, ignoring.\n");
    }
    else {
        
        if (fwrite(game, sizeof(gameContext), 1, f) == -1) {
            print("Unable to write to opened save file, ignoring.\n");
        }
        
        fclose(f); // Does a free(f) for you.
    }
}

