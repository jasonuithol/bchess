#!/bin/bash
# Profile-guided optimisation build.
#
# Two-pass: build instrumented, run a representative workload to gather
# branch / call-frequency profile data, then rebuild with the profile.
# The compiler uses the profile to inline aggressively along hot paths
# and to lay out branches the way they actually go in real searches.
#
# Worth ~15% wall-clock on this engine on top of -O3 -flto -march=native.
# Re-run whenever the search code changes substantially or you change
# CPU targets — old profile data on a different code shape can hurt.

set -eu

cd "$(dirname "$0")"

PROFILE_DIR="${PROFILE_DIR:-/tmp/bchess-pgo}"
rm -rf "$PROFILE_DIR"
mkdir -p "$PROFILE_DIR"

echo "[1/3] building instrumented binary"
./build.sh -fprofile-generate="$PROFILE_DIR" >/dev/null

echo "[2/3] gathering profile data"
# bench: representative search workload across the eval regimes the
# bench corpus was hand-picked to cover (opening / middlegame /
# simplification / endgame).
printf 'bench\nquit\n' | ./a.out >/dev/null
# perft: exercises move generation more heavily than search alone.
printf 'position startpos\nperft 5\nquit\n' | ./a.out >/dev/null
printf 'position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1\nperft 4\nquit\n' \
    | ./a.out >/dev/null
# real searches at varied positions.
printf 'position startpos moves e2e4 e7e5 g1f3 b8c6\ngo depth 7\nquit\n' | ./a.out >/dev/null
printf 'position startpos moves d2d4 d7d5 c2c4\ngo depth 7\nquit\n' | ./a.out >/dev/null

echo "[3/3] rebuilding with profile data"
./build.sh -fprofile-use="$PROFILE_DIR" >/dev/null

echo
echo "PGO build complete. Profile data in $PROFILE_DIR — safe to delete."
