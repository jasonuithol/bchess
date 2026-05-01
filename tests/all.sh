#!/bin/bash
# Run the full bchess regression suite. Builds first if no binary exists,
# then executes perft (move generation) and bench (search) checks.

set -u

cd "$(dirname "$0")/.."

if [ ! -x ./a.out ]; then
    echo "tests/all.sh: no binary, building first..."
    ./build.sh > /dev/null
fi

failures=0
./tests/perft.sh || failures=$((failures+1))
./tests/bench.sh || failures=$((failures+1))

if [ "$failures" = "0" ]; then
    echo
    echo "All checks passed."
fi
exit "$failures"
