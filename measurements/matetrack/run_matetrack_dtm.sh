#!/bin/bash
# matetrack with exact DTM at the root (chesstb 3-5 piece tables in chesstb), same setup as run_matetrack.sh.
export PYTHONPATH="python-chess"
MT=matetrack
V=.
TB=syzygy-3-4-5
DTM=chesstb
run() {
  local name=$1 eng=$2; shift 2
  local t0=$(date +%s)
  python $MT/matecheck.py --epdFile $MT/matetrack.epd --engine $eng --nodes 1000000 --concurrency 16 "$@" \
    > $V/matetrack-$name.txt 2>&1
  local rc=$?
  local res=$(grep -E 'Found mates|Best mates|Found TB wins|Bad|bad' $V/matetrack-$name.txt | tr -s ' ' | tr '\n' ' ')
  echo "$name: rc $rc, $(( $(date +%s) - t0 )) s | $res" | tee -a $V/summary.txt
  if [ $rc -ne 0 ] || ! grep -q 'Found mates' $V/matetrack-$name.txt; then echo "FAILED $name" | tee -a $V/summary.txt; exit 1; fi
}
echo "matetrack DTM runs, start $(date '+%F %H:%M')" >> $V/summary.txt
run F-filter-syzygy5-dtm      $V/filter-dtm.exe --syzygyPath $TB --engineOpts "{\"SyzygyProbeLimit\": 5, \"TBFilter\": true, \"DTMPath\": \"$DTM\"}"
run G-filter-no50-syz5-dtm    $V/filter-dtm.exe --syzygyPath $TB --engineOpts "{\"SyzygyProbeLimit\": 5, \"TBFilter\": true, \"Ignore50MoveRule\": true, \"DTMPath\": \"$DTM\"}"
run H-syzygy5-dtm             $V/filter-dtm.exe --syzygyPath $TB --engineOpts "{\"SyzygyProbeLimit\": 5, \"DTMPath\": \"$DTM\"}"
echo "DONE matetrack DTM $(date '+%H:%M')" | tee -a $V/summary.txt
