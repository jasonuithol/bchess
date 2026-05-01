#!/bin/bash
# Move-generation regression test.
#
# Runs bchess's "perft" command over a fixed set of (position, depth) pairs
# and verifies the leaf-node count matches a frozen baseline. Any change
# to move generation (make/unmake refactor, magic bitboards, en-passant
# implementation, etc.) will almost certainly produce a different count
# and trip these checks.
#
# All baseline values now match canonical perft (verified against
# python-chess via the chess-engine MCP). The suite covers:
#   * startpos at multiple depths
#   * Kiwipete — heavy castling, captures, multiple piece types
#   * "position 5" — promotions, knight on f2, pinned pieces
#   * KP endgame — basic pawn / king
#   * dedicated e.p. positions: a normal e.p. capture available,
#     and the famous "discovered check via e.p. capture" position
#     where the e.p. capture is illegal because removing both pawns
#     from rank 5 exposes the white king to a black rook.

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
startpos d=5	position startpos	5	4865609
Kiwipete d=1	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	1	48
Kiwipete d=2	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	2	2039
Kiwipete d=3	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	3	97862
Kiwipete d=4	position fen r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1	4	4085603
position-5 d=1	position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8	1	44
position-5 d=2	position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8	2	1486
position-5 d=3	position fen rnbq1k1r/pp1Pbppp/2p5/8/2B5/8/PPP1NnPP/RNBQK2R w KQ - 1 8	3	62379
position-3 d=5	position fen 8/2p5/3p4/KP5r/1R3p1k/8/4P1P1/8 w - - 0 1	5	674624
KP endgame d=4	position fen 8/8/4k3/8/8/2K5/4P3/8 w - - 0 1	4	5229
e.p. avail d=1	position fen rnbqkbnr/1ppppppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3	1	31
e.p. avail d=2	position fen rnbqkbnr/1ppppppp/p7/3pP3/8/8/PPPP1PPP/RNBQKBNR w KQkq d6 0 3	2	559
e.p. self-check d=1	position fen 8/8/8/KPp4r/1R3p1k/8/4P1P1/8 w - c6 0 2	1	16
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
