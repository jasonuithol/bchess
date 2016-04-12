#!/bin/bash

# I try to support both gcc and clang
COMPILER="gcc"

# We use anonymous nested structs.
LANGUAGE="-std=c11"

# Common sense.  Even warnings are treated as SERIOUS BUSINESS.
WARNINGS="-Wall -pedantic -Winline"

# We need to go fast
TARGET_ARCH="-march=native"

# We need to go even faster
OPTIMISE_clang="-O3 -flto"
OPTIMISE_gcc="-Ofast -flto -fuse-linker-plugin"
OPTIMISE_VAR="OPTIMISE_$COMPILER"

# Our "main" file.
INPUT="bbchess.c"

echo $COMPILER $LANGUAGE $WARNINGS $TARGET_ARCH ${!OPTIMISE_VAR} $* $INPUT

$COMPILER $LANGUAGE $WARNINGS $TARGET_ARCH ${!OPTIMISE_VAR} $* $INPUT
