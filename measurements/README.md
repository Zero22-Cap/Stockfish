# Zen 4 speed: pair int8 rows x NumRegistersSIMD 8

Measured by Zero22-Cap on an AMD Ryzen 7 7700X (Zen 4, 8 cores / 16 threads, 64 GB, Windows 11).

## Builds

All on master 5c4cef0b, GCC 16.1 (MSYS2 ucrt64), `make -j16 profile-build ARCH=x86-64-avx512icl COMP=mingw`.
Every build gives bench 1559611.

| Build | Source |
|---|---|
| master | 5c4cef0b |
| 8 registers only | 5c4cef0b + the one-line change in commit d5d15032 (from PR #7192) |
| pair int8 only | commit ab02ff1c (ayushthepiro11-design's patch, unchanged) |
| both | commit d5d15032 (this branch without the measurements commit) |

## Method

`speed_pair.py`, base and test interleaved in alternating order:
- A) one process at a time, pinned to one physical core, `bench 16 1 13` and `bench 16 1 16`
- B) 8 processes at once, one per physical core, half base and half test, swapped every round, `bench 16 1 16`

Error margins are 95% intervals. Day-to-day spread with the same binaries is larger than these
margins (the pair-only build measured +0.9 / +1.2% on 2026-10-15 and +2.0 / +1.7% on 2026-10-16).

## Results, 2026-10-16 (all against master)

| Build | 1 process, depth 13 | 1 process, depth 16 | 8 at once, depth 16 |
|---|---|---|---|
| 8 registers only | +3.11% ± 0.61 | +3.01% ± 0.63 | +2.53% ± 0.60 |
| pair int8 only | +1.98% ± 0.25 | +1.71% ± 0.52 | +0.60% ± 1.71 (noisy) |
| both | +2.95% ± 0.30 | +2.89% ± 0.52 | +2.55% ± 0.27 |

Raw log: `log-2026-10-16-four-builds.txt`.

## Direct comparisons, 2026-10-15

| Comparison | 1 process, depth 13 | 1 process, depth 16 | 8 at once, depth 16 |
|---|---|---|---|
| pair int8 vs master | +0.94% ± 0.29 | +1.23% ± 0.19 | +0.65% ± 0.18 |
| both vs 8 registers only | -0.76% ± 0.24 | -0.69% ± 0.16 | -0.60% ± 0.21 |

Raw log: `log-2026-10-15-pair-patch.txt`.

## Conclusion

On Zen 4 the pair int8 patch alone is faster than master, but on top of the NumRegistersSIMD 8
change it adds nothing: both together are about as fast as 8 registers alone (within noise on
2026-10-16, slightly slower in the direct comparison on 2026-10-15).
