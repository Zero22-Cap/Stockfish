"""Paired nps comparison of two Stockfish builds (same bench signature expected).

A) one process at a time, pinned to one physical core, base/test interleaved with alternating order
B) 8 processes at once on the 8 physical cores, half base and half test, swapped every round

usage: python speed_pair.py <base.exe> <test.exe> [pairs13 pairs16 rounds]
"""
import ctypes, math, re, statistics, subprocess, sys, time

BASE, TEST = sys.argv[1], sys.argv[2]
P13 = int(sys.argv[3]) if len(sys.argv) > 3 else 60
P16 = int(sys.argv[4]) if len(sys.argv) > 4 else 40
ROUNDS = int(sys.argv[5]) if len(sys.argv) > 5 else 20
k32 = ctypes.windll.kernel32
NPS = re.compile(r'Nodes/second\s*:\s*(\d+)')
NODES = re.compile(r'Nodes searched\s*:\s*(\d+)')
HIGH = 0x00000080  # HIGH_PRIORITY_CLASS


def start(exe, depth, cpu):
    p = subprocess.Popen([exe, 'bench', '16', '1', str(depth)], stdout=subprocess.DEVNULL,
                         stderr=subprocess.PIPE, text=True, creationflags=HIGH)
    k32.SetProcessAffinityMask(int(p._handle), 1 << cpu)
    return p


def finish(p):
    err = p.communicate()[1]
    return int(NPS.search(err).group(1)), int(NODES.search(err).group(1))


def report(tag, base, test, diffs):
    n = len(diffs)
    m = statistics.mean(diffs)
    ci = 1.96 * statistics.stdev(diffs) / math.sqrt(n)
    print(f'{tag}: n={n}  base {statistics.mean(base):,.0f} nps  test {statistics.mean(test):,.0f} nps  '
          f'speedup {m:+.3f}% +/- {ci:.3f}% (95%)', flush=True)


def serial(depth, pairs, cpu=4):
    base, test, diffs, sig = [], [], [], set()
    for i in range(pairs):
        order = (BASE, TEST) if i % 2 == 0 else (TEST, BASE)
        r = {}
        for exe in order:
            nps, nodes = finish(start(exe, depth, cpu))
            r[exe] = nps
            sig.add((exe == TEST, nodes))
        base.append(r[BASE]); test.append(r[TEST])
        diffs.append(100.0 * (r[TEST] - r[BASE]) / r[BASE])
    print(f'  depth {depth} node counts (is_test, nodes): {sorted(sig)}', flush=True)
    report(f'A) one at a time, bench 16 1 {depth}', base, test, diffs)


def concurrent(depth, rounds):
    cpus = [0, 2, 4, 6, 8, 10, 12, 14]          # one logical CPU per physical core
    last = {}
    base, test, diffs = [], [], []
    for r in range(rounds):
        procs = []
        for j, c in enumerate(cpus):
            exe = BASE if (j + r) % 2 == 0 else TEST
            procs.append((c, exe, start(exe, depth, c)))
        cur = {}
        for c, exe, p in procs:
            cur[c] = (exe, finish(p)[0])
        if last:
            for c in cpus:                        # same core, consecutive rounds, other build
                (e1, n1), (e2, n2) = last[c], cur[c]
                b, t = (n1, n2) if e1 == BASE else (n2, n1)
                base.append(b); test.append(t)
                diffs.append(100.0 * (t - b) / b)
        last = cur
    report(f'B) 8 at once (all physical cores busy), bench 16 1 {depth}', base, test, diffs)


t0 = time.time()
serial(13, P13)
serial(16, P16)
concurrent(16, ROUNDS)
print(f'total {time.time() - t0:.0f}s')
