"""Wall-clock time to the first correct-sign mate announcement, per position.

Usage: python ttm3.py <engine> <positions.jsonl> <seconds cap> <out.json> ['{"Opt": val}']
Positions: one JSON object per line with "fen" and "want" (mate length, sign = side to move wins).
One thread per position, 16 positions in parallel, so every configuration runs under the same load.
"""
import json, sys, threading
from concurrent.futures import ThreadPoolExecutor
import chess, chess.engine

engine_path, posfile, cap, out = sys.argv[1], sys.argv[2], float(sys.argv[3]), sys.argv[4]
opts = json.loads(sys.argv[5]) if len(sys.argv) > 5 else {}
rows = [json.loads(l) for l in open(posfile, encoding='utf-8') if l.strip()]


START_LOCK = threading.Lock()


def work(chunk):
    # The personal builds default to 16 threads and a 16 GB hash, allocated at start-up:
    # start the instances one at a time and shrink each before the next one starts.
    with START_LOCK:
        eng = chess.engine.SimpleEngine.popen_uci(engine_path, timeout=180)
        eng.configure(dict({'Hash': 512, 'Threads': 1}, **opts))
    res = []
    for p in chunk:
        b = chess.Board(p['fen'])
        first = nodes = mate = None
        with eng.analysis(b, chess.engine.Limit(time=cap), game=object()) as an:
            for info in an:
                sc = info.get('score')
                if sc is None:
                    continue
                m = sc.pov(b.turn).mate()
                if m is not None and (m > 0) == (p['want'] > 0):
                    first, nodes, mate = info.get('time'), info.get('nodes'), m
                    break
        res.append({'fen': p['fen'], 'want': p['want'], 'men': p.get('men'), 'time': first, 'nodes': nodes, 'mate': mate})
    eng.quit()
    return res


N = 16
with ThreadPoolExecutor(N) as ex:
    res = [r for c in ex.map(work, [rows[i::N] for i in range(N)]) for r in c]
json.dump(res, open(out, 'w'))
solved = [r for r in res if r['time'] is not None]
print(f'{len(res)} positions, solved {len(solved)} within {cap:.0f}s')
