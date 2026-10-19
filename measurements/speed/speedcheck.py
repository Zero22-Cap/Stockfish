"""Search speed on the 260 long endgames: fixed nodes per position, 1 thread per engine,
16 positions in parallel, nps = nodes / time from the last info line.
Usage: speedcheck.py <engine> <positions.jsonl> <nodes> <out.json> ['{"Opt": val}']"""
import json, sys, threading, statistics
from concurrent.futures import ThreadPoolExecutor
import chess, chess.engine

engine_path, posfile, nodes, out = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
opts = json.loads(sys.argv[5]) if len(sys.argv) > 5 else {}
rows = [json.loads(l) for l in open(posfile, encoding='utf-8') if l.strip()]
LOCK = threading.Lock()

def work(chunk):
    with LOCK:
        eng = chess.engine.SimpleEngine.popen_uci(engine_path, timeout=180)
        eng.configure(dict({'Hash': 512, 'Threads': 1}, **opts))
    res = []
    for p in chunk:
        b = chess.Board(p['fen'])
        info = eng.analyse(b, chess.engine.Limit(nodes=nodes), game=object())
        res.append({'fen': p['fen'], 'nodes': info.get('nodes'), 'time': info.get('time'),
                    'tbhits': info.get('tbhits', 0)})
    eng.quit()
    return res

N = 16
with ThreadPoolExecutor(N) as ex:
    res = [r for c in ex.map(work, [rows[i::N] for i in range(N)]) for r in c]
json.dump(res, open(out, 'w'))
nps = [r['nodes'] / r['time'] for r in res if r['time']]
print(f'{len(res)} positions, median nps {statistics.median(nps):.0f}, '
      f'total nodes {sum(r["nodes"] for r in res)}, total time {sum(r["time"] for r in res):.1f} s')
