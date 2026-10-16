# TBFilter on matetrack

Measured by Zero22-Cap on an AMD Ryzen 7 7700X, Windows 11.

## What the options do

- **TBFilter**: inside the tablebases, a drawn position returns a draw, and a move by the side that
  wins at the root which throws the win away is cut. Everything else is searched on to a real mate,
  without tablebase cutoffs. Only the winning side's moves are removed, so no mate is invented.
  WDL verdicts are cached per thread. `TBFilterDepth` (default 4) is the minimum depth for the check.
- **Ignore50MoveRule**: mates longer than the 50-move rule allows count, as in DTM tables.

Both are off by default; with them off the engine is identical to master (bench 1559611).

## Setup

- master 5c4cef0b and this branch (commit c15c677f), both built with GCC 16.1,
  `make -j profile-build ARCH=x86-64-avx512icl`
- vondele/matetrack `matecheck.py`, `matetrack.epd` (6554 positions), `--nodes 1000000`,
  1 thread per engine, `--concurrency 16`, default hash
- Syzygy 3-4-5 WDL+DTZ (a folder with only the 3-5 piece files), `SyzygyProbeLimit 5`

## Results

| Run | Found mates | Best mates | Found TB wins |
|---|---:|---:|---:|
| A. master, no TB | 4000 | 2699 | - |
| B. master, Syzygy | 3799 | 2687 | 430 |
| C. TBFilter, Syzygy | **3995** | **2730** | 8 |
| D. TBFilter + Ignore50MoveRule, Syzygy | 4002 | 2717 | 11 |
| E. Ignore50MoveRule, no TB | 3956 | 2686 | - |

- With plain Syzygy, 430 searches end at a TB win, and 201 fewer mates are found than without
  tablebases. With TBFilter only 8 end at a TB win, found mates are back to the no-TB level, and it
  finds the most best mates of all runs.
- Ignore50MoveRule on its own does not help on matetrack (run E).

Commands (engine and folder names shortened):

```
python matecheck.py --epdFile matetrack.epd --engine master/stockfish.exe --nodes 1000000 --concurrency 16
python matecheck.py ... --engine master/stockfish.exe --syzygyPath syzygy-3-4-5 --engineOpts '{"SyzygyProbeLimit": 5}'
python matecheck.py ... --engine tb-filter/stockfish.exe --syzygyPath syzygy-3-4-5 --engineOpts '{"SyzygyProbeLimit": 5, "TBFilter": true}'
python matecheck.py ... --engine tb-filter/stockfish.exe --syzygyPath syzygy-3-4-5 --engineOpts '{"SyzygyProbeLimit": 5, "TBFilter": true, "Ignore50MoveRule": true}'
python matecheck.py ... --engine tb-filter/stockfish.exe --engineOpts '{"Ignore50MoveRule": true}'
```

Raw outputs: `matetrack-*.txt`.

## Long tablebase endgames (earlier build, 2026-10-06)

260 positions with 4 to 7 pieces from the Syzygy max-DTZ list, all decisive, where engines found
mates of 21 to 89 moves (`long-endgames/positions.jsonl`; the mate lengths are upper bounds from
earlier searches, only their sign is used). For each position: wall time until the engine first
announces a mate for the winning side, 1 thread per engine, 16 positions in parallel, hash 512 MB
(`long-endgames/ttm3.py`). Syzygy: complete 6-piece set plus some 7-piece tables.

These runs used an earlier build of the same filter (same Step 7a logic and verdict cache) on
master 49ea5ded, inside a personal build whose other options were off. Per-position results are in
`long-endgames/*.json`.

| Setting | Solved in 20 s | Solved in 120 s |
|---|---:|---:|
| Syzygy (normal probing) | 23 | 35 |
| no TB | 29 | |
| TBFilter, Syzygy | 28 | |
| Ignore50MoveRule, Syzygy | 25 | |
| Ignore50MoveRule, no TB | 29 | 50 |
| TBFilter + Ignore50MoveRule, Syzygy | **33** | **53** |

In these long endgames normal Syzygy probing finds the fewest mates. Here, unlike on matetrack, the
combination with Ignore50MoveRule matters: many of these mates are longer than 50 moves.
