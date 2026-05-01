#!/bin/bash
# Move-generation regression test.
#
# Runs bchess's "perft" command over a fixed set of (position, depth) pairs
# and verifies the leaf-node count matches a frozen baseline. Any change
# to move generation (make/unmake refactor, magic bitboards, en-passant
# implementation, etc.) will almost certainly produce a different count
# and trip these checks.
#
# Notes on the baseline values:
#   * startpos d=1..4 match the canonical chess values exactly.
#   * startpos d=5 = 4_865_351 vs canonical 4_865_609. The 258 difference
#     is en-passant captures, which bchess does not currently generate.
#     If e.p. is ever implemented, update this expected value to match
#     canonical (4_865_609) — or, better, copy the canonical values for
#     all depths and drop the explanatory note.
#   * Kiwipete and "position 5" come from the standard Chess Programming
#     Wiki perft test set; the values below are bchess's current output,
#     not canonical (canonical includes en-passant lines).

set -u

cd "$(dirname "$0")/.."
ENGINE="${ENGINE:-./a.out}"

if [ ! -x "$ENGINE" ]; then
    echo "perft: engine binary not found at $ENGINE — run ./build.sh first" >&2
    exit 2
fi

# Tab-separated: <label> \t <position-cmd> \t <depth> \t <expected-nodes>
TESTS=$(cat <<'EOF'
startpos d=1	position startpos	1	20
startpos d=2	position startpos	2	400
startpos d=3	position startpos	3	8902
startpos d=4	position startpos	4	197281
startpos d=5	position startpos	5	4865351
Kiwipete d=1	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	1	48
Kiwipete d=2	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	2	2038
Kiwipete d=3	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	3	97766
position-5 d=1	position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8	1	44
position-5 d=2	position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8	2	1486
position-5 d=3	position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8	3	62416
KP endgame d=4	position fen 8/8/4k3/8/8/2K5/4P3/8 w - - 0 1	4	5229
EOF
)

pass=0
fail=0
while IFS=$'\t' read -r label posCmd depth expected; do
    actual=$(printf '%s\nperft %s\nquit\n' "$posCmd" "$depth" \
             | "$ENGINE" 2>&1 \
             | awk '/Nodes searched:/ {print $NF}')
    if [ "$actual" = "$expected" ]; then
        printf "  ok   %-20s %s\n" "$label" "$expected"
        pass=$((pass+1))
    else
        printf "  FAIL %-20s expected=%s actual=%s\n" "$label" "$expected" "$actual"
        fail=$((fail+1))
    fi
done <<< "$TESTS"

echo
echo "perft: $pass passed, $fail failed"
exit $((fail > 0))
