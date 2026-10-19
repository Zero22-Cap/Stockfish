# TBFilter and exact DTM: measurements

Measured by Zero22-Cap on an AMD Ryzen 7 7700X, Windows 11, 2026-10-19.

## What the options do

- **TBFilter**: inside the Syzygy tables, a drawn position returns a draw, and a move by the side that
  wins at the root which throws the win away is cut. Everything else is searched on to a real mate,
  without tablebase cutoffs. Only WDL is probed, never DTZ or DTM, and verdicts are cached per thread.
  `TBFilterDepth` (default 4) is the minimum depth for the check.
- **Ignore50MoveRule**: mates longer than the 50-move rule allows count, as in DTM tables.
- **DTMPath** (only in a build with `CHESSTB_DTM`, see below): chesstb DTM tables. When the root
  position is in them, every root move gets its exact result, the engine plays a DTM-optimal move and
  shows "mate N" with the line to mate from the tables. Nothing is probed inside the search. With the
  50-move rule on, a result is used only if the mate comes before the clock could reach 100.

All are off by default; with them off the engine is identical to master (bench 1559611).

## Builds

- master effdc3fd and this branch, GCC 16.1 (MSYS2 ucrt64),
  `make -j16 profile-build ARCH=x86-64-avx512icl COMP=mingw`
- the DTM build adds `CHESSTB_DTM=<path>/libchesstb_dtm.a`, built with
  `scripts/chesstb_dtm/build.sh <chesstb checkout>` from https://github.com/noobpwnftw/chesstb
  (public domain). Its bench with DTMPath empty is also 1559611.
- local tables: Syzygy 3-5 piece (matetrack) or the complete 6-piece set (long endgames);
  chesstb wdl + dtm for 3-5 pieces.

## matetrack

vondele/matetrack `matecheck.py`, `matetrack.epd` (6554 positions), `--nodes 1000000`, 1 thread per
engine, `--concurrency 16`, Syzygy 3-4-5, `SyzygyProbeLimit 5` (`matetrack/run_matetrack*.sh`).

| Run | Found mates | Best mates | Found TB wins |
|---|---:|---:|---:|
| A. master, no TB | 4000 | 2699 | - |
| B. master, Syzygy | 3799 | 2687 | 430 |
| C. TBFilter, Syzygy | 3995 | 2730 | 8 |
| D. TBFilter + Ignore50MoveRule, Syzygy | 4002 | 2717 | 11 |
| E. Ignore50MoveRule, no TB | 3956 | 2686 | - |
| F. TBFilter + DTM, Syzygy | 4003 | **2761** | 0 |
| G. TBFilter + Ignore50MoveRule + DTM, Syzygy | **4013** | 2755 | 0 |
| H. DTM only, Syzygy | 3808 | 2724 | 421 |

- A-E are node-limited and identical to the same runs on the previous master 5c4cef0b: the four new
  master commits change speed only.
- Most TB wins come from cutoffs inside the search, so DTM at the root alone (H) removes only the 9
  whose root is in the 5-piece tables. TBFilter removes the cutoffs, and DTM then converts the
  remaining wins at the root (F, G: no TB wins left, no bad PVs).
- 99 of the 6554 positions have 5 pieces or fewer; DTM answers those exactly.

Raw outputs: `matetrack/matetrack-*.txt`.

## Search speed

The 260 long endgames below, 3M nodes per position, 1 thread per engine, 16 at once, Syzygy 6-piece,
nps from the last info line (`speed/speedcheck.py`).

| Setting | Median nps | Total search time |
|---|---:|---:|
| no TB | 1,568,392 | 487.9 s |
| Syzygy | 1,233,058 | 641.2 s |
| TBFilter, Syzygy | 1,236,501 | 627.0 s |

Per position, TBFilter vs plain Syzygy: median ratio 1.014 (10%-90%: 0.83-1.28). The filter costs
nothing over normal Syzygy probing; both are about 20% slower than no TB on these positions.

## Long tablebase endgames

260 positions with 4 to 7 pieces from the Syzygy max-DTZ list, all decisive, where engines found
mates of 21 to 89 moves (`long-endgames/positions.jsonl`; the mate lengths are upper bounds from
earlier searches, only their sign is used). For each position: wall time until the engine first
announces a mate for the winning side, cap 60 s, 1 thread per engine, 16 positions in parallel,
hash 512 MB (`long-endgames/ttm3.py`). Syzygy: complete 6-piece set plus some 7-piece tables.

| Setting | Solved in 20 s | Solved in 60 s | of which 4-5 pieces (17) | 6-7 pieces (243) |
|---|---:|---:|---:|---:|
| A. master, Syzygy | 23 | 31 | 6 | 25 |
| B. master, no TB | 28 | 36 | 5 | 31 |
| C. TBFilter, Syzygy | 30 | 34 | 4 | 30 |
| D. TBFilter + Ignore50MoveRule, Syzygy | 31 | 44 | 6 | 38 |
| E. Ignore50MoveRule, no TB | 30 | 38 | 5 | 33 |
| F. D + DTM | 44 | 55 | 17 | 38 |

(the last two columns are for 60 s)

- Normal Syzygy probing finds the fewest mates. TBFilter with Ignore50MoveRule finds the most from
  search (many of these mates are longer than 50 moves).
- DTM adds the 4-5 piece positions, which it answers from the tables at once; at 6-7 pieces F and D
  are the same, as expected since DTM is only used at the root.

Per-position results: `long-endgames/*.json`; `analyze.py` prints both tables' derived numbers.
All runs in order: `summary.txt`.
