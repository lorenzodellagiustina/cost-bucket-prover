#!/bin/bash
# Tests of the cost-counter pass alone, without SeaHorn: builds the pass, instruments the test
# programs, runs them natively, and checks the block weights and the measured costs. Needs LLVM 14
# (clang-14, opt-14, llvm-link-14) and CMake, as in the dev container:
#   tests/run_pass.sh
set -uo pipefail
ROOT=$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)
OUT=$ROOT/tests/out/pass
BUILD=$ROOT/pass/build
rm -rf "$OUT" && mkdir -p "$OUT"
failures=0

fail() { echo "FAIL: $*"; failures=$((failures + 1)); }

cmake -S "$ROOT/pass" -B "$BUILD" -G Ninja -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_C_COMPILER=clang-14 -DCMAKE_CXX_COMPILER=clang++-14 \
  -DLLVM_DIR=/usr/lib/llvm-14/lib/cmake/llvm -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_INSTALL_PREFIX="$BUILD/install" >"$OUT/cmake.log" 2>&1 &&
  cmake --build "$BUILD" >>"$OUT/cmake.log" 2>&1 &&
  cmake --install "$BUILD" >>"$OUT/cmake.log" 2>&1 ||
  { echo "FAIL: building the pass (see $OUT/cmake.log)"; exit 1; }
export PREFIX=$BUILD/install

# instrument NAME TOP SRC [CFLAGS]: instrumented IR at $OUT/NAME.ll, report at $OUT/NAME.yaml.
instrument() {
  SRCS=$3 TOP_FUNCTION=$2 CFLAGS=${4:-} OUTPUT="$OUT/$1.ll" REPORT="$OUT/$1.yaml" \
    "$ROOT/scripts/instrument.sh" >"$OUT/$1.instrument.log" 2>&1 ||
    { fail "$1: instrument (see $OUT/$1.instrument.log)"; return 1; }
}

# native NAME DRIVER: runs DRIVER linked with $OUT/NAME.ll, output at $OUT/NAME.costs.
native() {
  clang-14 -O0 "$2" "$OUT/$1.ll" -o "$OUT/$1.exe" >"$OUT/$1.native.log" 2>&1 &&
    "$OUT/$1.exe" >"$OUT/$1.costs" ||
    { fail "$1: native run (see $OUT/$1.native.log)"; return 1; }
}

# --- loop3 at -O0: exact block weights, cost 13n + 13 -----------------------------------------
if instrument loop3 loop3 "$ROOT/tests/loop3/loop3.c" && native loop3 "$ROOT/tests/loop3/native.c"; then
  weights=$(python3 -c 'import sys, yaml; print(" ".join(str(b["weight"]) for f in yaml.safe_load(open(sys.argv[1]))["functions"] for b in f["blocks"]))' "$OUT/loop3.yaml")
  [ "$weights" = "7 4 5 4 2" ] || fail "loop3: block weights '$weights', expected '7 4 5 4 2'"
  while read -r n cost; do
    [ "$cost" -eq $((13 * n + 13)) ] || fail "loop3: cost($n) = $cost, expected $((13 * n + 13))"
  done <"$OUT/loop3.costs"
  echo "loop3 (-O0): weights $weights; costs" $(awk '{ printf "%s:%s ", $1, $2 }' "$OUT/loop3.costs")
fi

# --- collatz at -O1: blocks start with phi nodes, the increment must go after them -------------
# opt verifies the module it writes, so an increment before a phi fails `instrument`.
if instrument collatz collatz "$ROOT/tests/phi/collatz.c" -O1 && native collatz "$ROOT/tests/phi/native.c"; then
  grep -q ' = phi ' "$OUT/collatz.ll" || fail "collatz (-O1): no phi node, the case is not exercised"
  # Every iteration runs the same blocks, so the cost is affine in the number of steps.
  per_step=$(awk '$3 == 0 { c0 = $2 } $3 > 0 { print ($2 - c0) / $3 }' "$OUT/collatz.costs" | sort -u)
  [ "$(echo "$per_step" | wc -l)" -eq 1 ] || fail "collatz (-O1): cost not affine in the steps ($per_step)"
  echo "collatz (-O1): phi nodes present; n:cost:steps" \
    $(awk '{ printf "%s:%s:%s ", $1, $2, $3 }' "$OUT/collatz.costs") "($per_step per step)"
fi

# --- calls: every defined function counted, however it is called ------------------------------
if instrument calls top "$ROOT/tests/calls/calls.c" && native calls "$ROOT/tests/calls/native.c"; then
  python3 - "$OUT/calls.yaml" <<'EOF' || fail "calls: instrumentation report"
import sys, yaml
r = yaml.safe_load(open(sys.argv[1]))
names = {f["name"] for f in r["functions"]}
assert {"top", "helper", "init", "twice"} <= names, names  # every defined function
assert r["indirect_calls"] == 1, r["indirect_calls"]
assert r["uncounted_calls"] == {"external": {}, "inline_asm": 0}, r["uncounted_calls"]
EOF
  op=$(awk '$1 == "op" { print $2 }' "$OUT/calls.costs")
  [ "$op" -gt 0 ] || fail "calls: the callee reached only through a pointer is not counted"
  # Each extra iteration of helper's loop adds the same cost, whatever init() ran before.
  deltas=$(awk '$1 ~ /^[0-9]+$/ { if (NR > 1) print $2 - c; c = $2 }' "$OUT/calls.costs" | sort -u | wc -l)
  [ "$deltas" -eq 1 ] || fail "calls: cost(n + 1) - cost(n) is not constant"
  echo "calls: twice counted (cost $op through the pointer); costs" \
    $(awk '$1 ~ /^[0-9]+$/ { printf "%s:%s ", $1, $2 }' "$OUT/calls.costs")
fi

echo
[ $failures -eq 0 ] && echo "ALL PASS TESTS PASSED" || { echo "$failures FAILURE(S)"; exit 1; }
