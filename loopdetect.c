#define MAX_LOOP_DETECT (3)

typedef struct {
    board b;
    byte count; 

} loopDetect;

typedef struct {
    loopDetect items[MAX_LOOP_DETECT];
    byte ix;
    
} loopDetectList;

void initLoopDetect(loopDetectList* list) {
    memset((void*)list, 0, sizeof(loopDetectList));
}

byte searchLoopDetect(loopDetectList* list, board* b) {
    for (byte i = 0; i < MAX_LOOP_DETECT; i++) {
        if (areEqualQB(b, list->items[i]) {
            return i;
        }
    }
    return MAX_LOOP_DETECT;
}

void addLoopDetect(loopDetectList* list, board* b) {
    if (list->ix >= MAX_LOOP_DETECT) {
        list->ix = 0;
    }
    list->items[list->ix].count = 0;
    memcpy((void*)&(list->items[list->ix].b), (void*)b, sizeof(board));
}

byte incrementLoopDetect(loopDetectList* list, byte i) {
    if (i >= MAX_LOOP_DETECT) {
        error("Tried to access out of range of loop detect\n");
    }
    else {
        list->items[i].count++;
        return list->items[i].count;
    }
}

byte checkForLoops(loopDetectList* list, board* b) {
    byte i = searchLoopDetect(list, b);
    if (i == MAX_LOOP_DETECT) {
        return 0;
    }
    else {
        
    }
}
