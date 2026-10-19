#!/bin/bash
# matetrack, 6554 positions, 1M nodes, 1 thread per engine, 16 at once, Syzygy 3-5 (syzygy-3-4-5).
# Engines built from master effdc3fd and tb-filter-v2 (same recipe). Stops on the first failed run.
export PYTHONPATH="python-chess"
MT=matetrack
V=.
TB=syzygy-3-4-5
run() {  # name engine [extra args...]
  local name=$1 eng=$2; shift 2
  local t0=$(date +%s)
  python $MT/matecheck.py --epdFile $MT/matetrack.epd --engine $eng --nodes 1000000 --concurrency 16 "$@" \
    > $V/matetrack-$name.txt 2>&1
  local rc=$?
  local res=$(grep -E 'Found mates|Best mates|Found TB wins' $V/matetrack-$name.txt | tr -s ' ' | tr '\n' ' ')
  echo "$name: rc $rc, $(( $(date +%s) - t0 )) s | $res" | tee -a $V/summary.txt
  if [ $rc -ne 0 ] || ! grep -q 'Found mates' $V/matetrack-$name.txt; then echo "FAILED $name" | tee -a $V/summary.txt; exit 1; fi
}
echo "matetrack 1M nodes on effdc3fd, start $(date '+%F %H:%M')" >> $V/summary.txt
run A-master-noTB        $V/master.exe
run B-master-syzygy5     $V/master.exe --syzygyPath $TB --engineOpts '{"SyzygyProbeLimit": 5}'
run C-filter-syzygy5     $V/filter.exe --syzygyPath $TB --engineOpts '{"SyzygyProbeLimit": 5, "TBFilter": true}'
run D-filter-no50-syz5   $V/filter.exe --syzygyPath $TB --engineOpts '{"SyzygyProbeLimit": 5, "TBFilter": true, "Ignore50MoveRule": true}'
run E-no50-noTB          $V/filter.exe --engineOpts '{"Ignore50MoveRule": true}'
echo "DONE $(date '+%H:%M')" | tee -a $V/summary.txt
