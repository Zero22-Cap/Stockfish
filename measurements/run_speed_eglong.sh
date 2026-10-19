#!/bin/bash
# 1) search speed on the 260 long endgames (fixed 3M nodes), 2) time to mate on the same set (60 s cap).
# Syzygy: syzygy-6 (complete 6-piece set plus some 7-piece tables), probed read-only.
# Stops on the first failed run.
export PYTHONPATH="python-chess"
V=.
POS=long-endgames/positions.jsonl
TB=syzygy-6
M=$V/master.exe
B=$V/filter-dtm.exe
check() { if [ $1 -ne 0 ] || ! grep -q "$2" $3; then echo "FAILED $4" | tee -a $V/summary.txt; exit 1; fi; }
speed() {  # name engine options
  local t0=$(date +%s)
  python $V/speedcheck.py $2 $POS 3000000 $V/speed-$1.json "$3" > $V/speed-$1.log 2>&1
  check $? 'median nps' $V/speed-$1.log $1
  echo "speed $1: $(( $(date +%s) - t0 )) s | $(tail -1 $V/speed-$1.log)" | tee -a $V/summary.txt
}
ttm() {  # name engine options
  local t0=$(date +%s)
  python matetrack/ttm3.py $2 $POS 60 $V/eglong-$1.json "$3" > $V/eglong-$1.log 2>&1
  check $? 'solved' $V/eglong-$1.log $1
  echo "eg-long $1: $(( $(date +%s) - t0 )) s | $(tail -1 $V/eglong-$1.log)" | tee -a $V/summary.txt
}
echo "speed + eg-long on effdc3fd, start $(date '+%F %H:%M')" >> $V/summary.txt
speed noTB          $M '{}'
speed syzygy6       $M "{\"SyzygyPath\": \"$TB\", \"SyzygyProbeLimit\": 6}"
speed filter-syz6   $B "{\"SyzygyPath\": \"$TB\", \"SyzygyProbeLimit\": 6, \"TBFilter\": true}"
ttm A-master-syzygy6    $M "{\"SyzygyPath\": \"$TB\", \"SyzygyProbeLimit\": 6}"
ttm B-master-noTB       $M '{}'
ttm C-filter-syzygy6    $B "{\"SyzygyPath\": \"$TB\", \"SyzygyProbeLimit\": 6, \"TBFilter\": true}"
ttm D-filter-no50-syz6  $B "{\"SyzygyPath\": \"$TB\", \"SyzygyProbeLimit\": 6, \"TBFilter\": true, \"Ignore50MoveRule\": true}"
ttm E-no50-noTB         $B '{"Ignore50MoveRule": true}'
ttm F-filter-no50-syz6-dtm $B "{\"SyzygyPath\": \"$TB\", \"SyzygyProbeLimit\": 6, \"TBFilter\": true, \"Ignore50MoveRule\": true, \"DTMPath\": \"chesstb\"}"
echo "DONE speed + eg-long $(date '+%H:%M')" | tee -a $V/summary.txt
