"""Summaries for the README: speed ratios per position, and eg-long solved counts by time and piece count."""
import json, statistics, os, chess
V = os.path.dirname(os.path.abspath(__file__))
def load(n): return json.load(open(os.path.join(V, n)))

sp = {k: {r['fen']: r for r in load(f'speed/{k}.json')} for k in ('noTB', 'syzygy6', 'filter-syz6')}
for k in ('syzygy6', 'filter-syz6'):
    rs = [(sp[k][f]['nodes'] / sp[k][f]['time']) / (sp['noTB'][f]['nodes'] / sp['noTB'][f]['time']) for f in sp['noTB']]
    print(f'speed {k} / noTB: median {statistics.median(rs):.3f}, mean {statistics.mean(rs):.3f}')
rs = [(sp['filter-syz6'][f]['nodes'] / sp['filter-syz6'][f]['time']) / (sp['syzygy6'][f]['nodes'] / sp['syzygy6'][f]['time']) for f in sp['noTB']]
print(f'speed filter / syzygy6: median {statistics.median(rs):.3f}, mean {statistics.mean(rs):.3f}, '
      f'10%..90% {sorted(rs)[len(rs)//10]:.3f}..{sorted(rs)[len(rs)*9//10]:.3f}')
for k in ('noTB', 'syzygy6', 'filter-syz6'):
    print(f'  {k}: median nps {statistics.median(r["nodes"]/r["time"] for r in sp[k].values()):.0f}, '
          f'total time {sum(r["time"] for r in sp[k].values()):.1f} s')

names = ['A-master-syzygy6', 'B-master-noTB', 'C-filter-syzygy6', 'D-filter-no50-syz6', 'E-no50-noTB', 'F-filter-no50-syz6-dtm']
print('\neg-long: solved within 20 s / 60 s (all 260; 4-5 pieces 17; 6-7 pieces 243)')
for n in names:
    p = os.path.join(V, f'long-endgames/{n}.json')
    if not os.path.exists(p): print(n, 'missing'); continue
    res = load(f'long-endgames/{n}.json')
    def cnt(t, lo, hi): return sum(1 for r in res if r['time'] is not None and r['time'] <= t
                                   and lo <= len(chess.Board(r['fen']).piece_map()) <= hi)
    print(f'{n:28s} 20s {cnt(20,0,9):3d}  60s {cnt(60,0,9):3d}   | <=5: {cnt(20,0,5)}/{cnt(60,0,5)}   6-7: {cnt(20,6,9)}/{cnt(60,6,9)}')
