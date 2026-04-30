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
#include "uci.h"

int main(void) {
    // Run in UCI mode - no terminal graphics, just text protocol
    uciLoop();
    return 0;
}
