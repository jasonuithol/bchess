#!/bin/bash

# I try to support both gcc and clang
COMPILER="${COMPILER:-clang}"
if ! command -v "$COMPILER" >/dev/null 2>&1; then
    COMPILER="gcc"
fi

# We use anonymous nested structs. _POSIX_C_SOURCE exposes clock_gettime
# under -std=c11 (which would otherwise hide POSIX-only declarations).
LANGUAGE="-std=c11 -D_POSIX_C_SOURCE=200809L"

# Common sense.  Even warnings are treated as SERIOUS BUSINESS.
WARNINGS="-Wall -pedantic"

# We need to go fast
TARGET_ARCH="-march=native"

# We need to go even faster
OPTIMISE_clang="-O3 -flto"
OPTIMISE_gcc="-Ofast -flto -fuse-linker-plugin"
OPTIMISE_VAR="OPTIMISE_$COMPILER"

# Our "main" file.
DEFAULT_MAIN_FILE="bchessuci.c"

if [ -n "$MAIN_FILE" ]; then
    echo "Using MAIN_FILE=$MAIN_FILE"
else
    echo using default main file
    MAIN_FILE=$DEFAULT_MAIN_FILE
fi

# All non-main translation units. The main wrapper (bbchess.c or bchessuci.c)
# is supplied by $MAIN_FILE; it provides main() and pulls in headers only.
SOURCES="logging.c bitboard.c iterator.c quadboard.c attacks.c slider.c umpire.c savegame.c aileaf.c moveordering.c tt.c ai2.c airoot.c human.c uci.c"

# Test/perf TUs are only included when BUILD_TESTING is in the args.
if [[ " $* " == *" -DBUILD_TESTING "* ]]; then
    SOURCES="$SOURCES test.c performance.c"
fi

echo $COMPILER $LANGUAGE $WARNINGS $TARGET_ARCH ${!OPTIMISE_VAR} $* $MAIN_FILE $SOURCES

$COMPILER $LANGUAGE $WARNINGS $TARGET_ARCH ${!OPTIMISE_VAR} $* $MAIN_FILE $SOURCES
