#!/bin/bash

# I try to support both gcc and clang
COMPILER="clang"

# We use anonymous nested structs.
LANGUAGE="-std=c11"

# Common sense.  Even warnings are treated as SERIOUS BUSINESS.
WARNINGS="-Wall -pedantic"

# We need to go fast
TARGET_ARCH="-march=native"

# We need to go even faster
OPTIMISE_clang="-O3 -flto"
OPTIMISE_gcc="-Ofast -flto -fuse-linker-plugin"
OPTIMISE_VAR="OPTIMISE_$COMPILER"

# Our "main" file.
#DEFAULT_MAIN_FILE="bbchess.c"
DEFAULT_MAIN_FILE="bchessuci.c"

if [ -n "$MAIN_FILE" ]; then
    echo "Using MAIN_FILE=$MAIN_FILE"
else
    echo using default main file
    MAIN_FILE=$DEFAULT_MAIN_FILE    
fi

echo $COMPILER $LANGUAGE $WARNINGS $TARGET_ARCH ${!OPTIMISE_VAR} $* $MAIN_FILE

$COMPILER $LANGUAGE $WARNINGS $TARGET_ARCH ${!OPTIMISE_VAR} $* $MAIN_FILE
