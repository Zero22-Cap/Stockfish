#!/bin/bash
# Builds libchesstb_dtm.a from a chesstb checkout plus glue.cpp, for
#   make profile-build ... CHESSTB_DTM=<path to libchesstb_dtm.a>
# Usage: build.sh <chesstb source dir> [output dir]
set -euo pipefail
CTB=$(cd "$1" && pwd)
OUT=$(mkdir -p "${2:-.}" && cd "${2:-.}" && pwd)
HERE=$(cd "$(dirname "$0")" && pwd)
OBJ="$OUT/obj"
rm -rf "$OBJ"
mkdir -p "$OBJ"
FLAGS="-O3 -DNDEBUG -march=native -w -I$CTB/src -I$CTB/lib"
objs=()
for f in "$CTB"/lib/lz4/*.c "$CTB"/lib/LZMA/*.c "$CTB"/lib/zstd/common/*.c \
         "$CTB"/lib/zstd/compress/*.c "$CTB"/lib/zstd/dictBuilder/*.c; do
  o="$OBJ/c_$(basename "$(dirname "$f")")_$(basename "$f" .c).o"
  gcc $FLAGS -x c -c "$f" -o "$o" &
  objs+=("$o")
done
for f in "$CTB"/src/chess/*.cpp "$CTB"/src/util/*.cpp "$CTB"/src/probe/*.cpp "$HERE/glue.cpp"; do
  o="$OBJ/cpp_$(basename "$(dirname "$f")")_$(basename "$f" .cpp).o"
  g++ -std=c++17 $FLAGS -c "$f" -o "$o" &
  objs+=("$o")
done
wait
for o in "${objs[@]}"; do [ -f "$o" ] || { echo "compile failed: $o"; exit 1; }; done
rm -f "$OUT/libchesstb_dtm.a"
ar rcs "$OUT/libchesstb_dtm.a" "${objs[@]}"
echo "built $OUT/libchesstb_dtm.a"
