#!/bin/bash
# Search regression test.
#
# Runs bchess's "bench" command — a fixed-depth search across a hardcoded
# corpus of positions baked into uci.c — and verifies the total node
# count and per-position best moves match a frozen baseline.
#
# Any change that affects search behaviour (alpha-beta ordering, TT
# replacement policy, eval calculation, etc.) will produce different
# numbers. That's the point: this catches regressions perft can't,
# because perft only validates move generation.
#
# If you intentionally change search behaviour, re-run `./a.out` with
# the bench command and update EXPECTED_TOTAL + EXPECTED_BESTMOVES.

set -u

cd "$(dirname "$0")/.."
ENGINE="${ENGINE:-./a.out}"

if [ ! -x "$ENGINE" ]; then
    echo "bench: engine binary not found at $ENGINE — run ./build.sh first" >&2
    exit 2
fi

# Frozen baseline from current master. Each line: position-index expected-bestmove
EXPECTED_TOTAL=5592659
EXPECTED_BESTMOVES=$(cat <<'EOF'
0 e2e3
1 b1c3
2 d4e5
3 e2e4
4 e1e2
EOF
)

output=$(printf 'bench\nquit\n' | "$ENGINE" 2>&1)

pass=0
fail=0

# Per-position bestmove checks.
while read -r idx expected; do
    actual=$(echo "$output" \
             | awk -v i="$idx" '
                 $1 == "#" && $2 == "bench[" i "]:" {
                     for (j = 1; j <= NF; j++) {
                         if ($j ~ /^bestmove=/) {
                             sub(/^bestmove=/, "", $j); print $j
                         }
                     }
                 }')
    if [ "$actual" = "$expected" ]; then
        printf "  ok   bench[%s]   %s\n" "$idx" "$expected"
        pass=$((pass+1))
    else
        printf "  FAIL bench[%s]   expected=%s actual=%s\n" "$idx" "$expected" "$actual"
        fail=$((fail+1))
    fi
done <<< "$EXPECTED_BESTMOVES"

# Total node-count check.
actualTotal=$(echo "$output" | awk '/^Nodes:/ {print $NF}')
if [ "$actualTotal" = "$EXPECTED_TOTAL" ]; then
    printf "  ok   total-nodes  %s\n" "$EXPECTED_TOTAL"
    pass=$((pass+1))
else
    printf "  FAIL total-nodes  expected=%s actual=%s\n" "$EXPECTED_TOTAL" "$actualTotal"
    fail=$((fail+1))
fi

echo
echo "bench: $pass passed, $fail failed"
exit $((fail > 0))
