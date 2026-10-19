# Zen 4 speed: NumRegistersSIMD 8 vs 16 on master effdc3fd

Measured by Zero22-Cap on an AMD Ryzen 7 7700X (Zen 4, 8 cores / 16 threads, 64 GB, Windows 11),
2026-10-19.

Master has moved on since the earlier numbers in #7192: effdc3fd includes the pair int8 rows patch,
lazy threats and the FullThreats LUT changes. Re-measured on top of it.

## Builds

GCC 16.1 (MSYS2 ucrt64), `make -j16 profile-build ARCH=x86-64-avx512icl COMP=mingw`, both bench 1559611.

| Build | Source | NumRegistersSIMD |
|---|---|---|
| base | master effdc3fd | 16 |
| test | this branch (master + the x86-64-zen5 target commit), built for x86-64-avx512icl | 8 |

## Method

`speed_pair.py`, base and test interleaved in alternating order:
- A) one process at a time, pinned to one physical core, `bench 16 1 13` (60 pairs) and `bench 16 1 16` (40 pairs)
- B) 8 processes at once, one per physical core, half base and half test, swapped every round, `bench 16 1 16` (20 rounds)

Error margins are 95% intervals. Day-to-day spread with the same binaries has been larger than these
margins before (see the zen4-pair-x-registers branch).

## Results

| | 1 process, depth 13 | 1 process, depth 16 | 8 at once, depth 16 |
|---|---|---|---|
| 8 vs 16 registers | +0.48% ± 0.28 | +0.85% ± 0.19 | +0.85% ± 0.30 |

On the previous master 5c4cef0b the same comparison gave about +3%. With pair int8 rows merged, 8
registers are still faster on Zen 4, but by under 1%.

Raw log: `log-2026-10-19-zen4-8-vs-16.txt`.
