# bchess — next session

Snapshot at end of session before forced reboot. Pick up here.

## Where we are

bchess (master, post-PGO) is fully UCI-compliant, plays a clean game against
PyChess, and the user reports it "totally holds its own" in human-vs-machine.

What's done in this session (chronological):
- Move-gen verified correct via `tests/perft.sh` (17 assertions, all match
  canonical perft).
- En passant implemented (was missing entirely — found via perft startpos d=5
  diverging from canonical 4_865_609).
- Corner-rook capture now correctly invalidates castling — found via Kiwipete
  d=4 / position-4 d=4 perft mismatches.
- PEXT bitboards for slider attack lookup (~30% wall-clock win on Zen4).
- Aspiration windows on the ID loop (~10-15% nodes per iter).
- TT bumped 1M → 2M entries (64 MB; fits the 7800X3D L3).
- PGO build via `./build-pgo.sh` (+15% on top).
- UCI compatibility: under-promotion, `stop`, `ponder`, quiet startup —
  i.e. `printMoveUCI` honours `move->promoteTo`, the position-handler matcher
  requires exact `promoteTo` agreement, and we own stdin via raw `read(2)` so
  the search can poll for `stop` mid-iteration via `pollSearchInput()`.

What's NOT on master (and probably won't be):
- Multithreading. Lives on the `multithreading` branch. Measured at +13 Elo
  for 1t→8t at 5+0.05 — within noise. Mobility eval is the bottleneck;
  threading didn't help.
- Score-delta quiescence. Reverted; lost ~58 Elo in self-play.

## Where we're going: Lichess bot

The user wants a real Elo number. Stockfish-throttled is too cruel a
benchmark — bchess went 0/60 against Stockfish at Skill Level 0 (the
weakest stockfish can be made), suggesting bchess is below ~900 CCRL,
which DOES NOT match its human-vs-machine performance. Humans make
different mistakes than crippled-Stockfish; engines that beat humans
can still lose 0/60 to skill-0 Stockfish. CCRL Elo and "fun to play
against" are different axes.

A Lichess bot account gives a real rating mixed across human + bot
opponents at the bchess-relevant strength range.

### Plan

1. **Create a Lichess account.** Free. Then
   `Account → Settings → Account preferences` and somewhere in there
   is the option to upgrade to a bot (or it requires
   `https://lichess.org/api/bot/account/upgrade` via API after the
   account has zero rated games).
2. **Generate an API token** at `lichess.org/account/oauth/token`.
   Scope: `bot:play`.
3. **Install [lichess-bot](https://github.com/lichess-bot-devs/lichess-bot)**.
   Python client. Reads a YAML config that points at a UCI engine binary
   and forwards Lichess game events to it.
4. **Point it at bchess.** Config snippet (rough — verify against
   the lichess-bot README, this is from memory):
   ```yaml
   token: "<your token>"
   url: "https://lichess.org/"
   engine:
     dir: "/home/jason/Projects/bchess/"
     name: "a.out"
     protocol: "uci"
     ponder: true
   challenge:
     concurrency: 1
     accept_bot: true
     accept_human: true
     variants: ["standard"]
     time_controls: ["bullet", "blitz", "rapid"]
   ```
5. **Run** `python3 lichess-bot.py` and let it accept challenges.
   After ~20-30 rated games the bot's Lichess rating stabilises.
6. **Watch the first few games** to make sure nothing's broken in the
   handshake. Common Lichess-bot pitfalls: variant mismatch, wrong
   protocol, the bot account not yet upgraded.

### Things to verify before deploying

- The PGO binary at `~/Projects/bchess/a.out` is the latest. If in doubt,
  `cd ~/Projects/bchess && ./build-pgo.sh && ./tests/all.sh`.
- Time-control behaviour. Lichess bullet (1+0) gives bchess ~33 ms/move.
  bchess respects this via the deadline abort but at that budget it might
  only reach depth 3-4. Probably fine; cross our fingers.
- bchess does NOT understand `stop` from stdin while idle — only during
  a search. lichess-bot might send commands the engine doesn't expect
  (e.g. `position fen ... moves ...` mid-game). Worth scanning the lichess-bot
  source if anything looks off.

### Open questions for tomorrow-you

- Should the bot account have a name like "bchess-bot" or
  "Tarbaby-bchess"? (Trivial; pick whatever.)
- Does Lichess-bot support pondering? If yes, our new ponder code gets
  exercised in the wild. Worth keeping an eye on the first games.
- Want to log every game to disk for offline analysis? lichess-bot has a
  PGN-output config option.

## State of the world

- Working tree on `master`, clean.
- Binary `./a.out` is the PGO build.
- chess-engine MCP has `bchess` and `stockfish` registered (plus the
  weak-stockfish variants we created during the Elo experiment —
  `sf-1500`, `sf-1350`, `sf-skill0`. Free to leave or unregister.)
- Test suite runs in ~10 s: `./tests/all.sh`.
