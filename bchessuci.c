// ==================================================================
//
// bchessuci.c
//
// UCI protocol entry point for bchess engine
//
// Author:      Jason Uithol
// Copyright:   2016
//
// ==================================================================

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "logging.c"
#include "bitboard.c"
#include "iterator.c"
#include "quadboard.c"
#include "attacks.c"
#include "umpire.c"
#include "savegame.c"
#include "aileaf.c"
#include "moveordering.c"
#include "ai2.c"
#include "airoot.c"
#include "human.c"
#include "uci.c"

int main(void) {
    // Run in UCI mode - no terminal graphics, just text protocol
    uciLoop();
    return 0;
}
