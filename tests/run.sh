#!/bin/bash
# End-to-end tests of the image's make interface. Run from the repository root:
#   docker run --rm --platform linux/amd64 -v "$PWD/tests":/data/tests \
#     --entrypoint bash cost-bucket-prover:<ref> tests/run.sh
set -uo pipefail
cd /data
M="make -s -f $PREFIX/Makefile"
OUT=tests/out
rm -rf "$OUT" && mkdir -p "$OUT"
failures=0

fail() { echo "FAIL: $*"; failures=$((failures + 1)); }
field() { awk -v k="$1:" '$1 == k { print $2 }' "$2"; }

# check NAME LO HI EXPECTED_VERDICT [EXPECTED_COST]
# For a reachable bucket, the counterexample is replayed. When the counterexample determines every
# input (nothing unrecorded), the replayed cost must lie in the bucket: anything else is a bug.
check() {
  local name=$1 lo=$2 hi=$3 want=$4 want_cost=${5:-}
  local p="$OUT/$CASE.$name"
  $M prove HARNESS="$HARNESS" IR="$IR" LO="$lo" ${hi:+HI="$hi"} INCLUDES="$INCLUDES" \
    TIMEOUT="${TIMEOUT:-60}" OUTPUT="$p.prove.yaml"
  local got; got=$(field verdict "$p.prove.yaml")
  local secs; secs=$(field seconds "$p.prove.yaml")
  local line="$CASE [$lo,${hi:-inf}) verdict=$got (${secs}s)"
  [ "$got" = "$want" ] || fail "$line, expected $want"
  if [ "$got" = reachable ]; then
    $M replay HARNESS="$HARNESS" IR="$IR" CEX="$(field cex "$p.prove.yaml")" \
      INCLUDES="$INCLUDES" OUTPUT="$p.replay.yaml"
    local st cost unrec
    st=$(field status "$p.replay.yaml")
    cost=$(field cost "$p.replay.yaml")
    unrec=$(awk '/^unrecorded:/ { f = 1; next } f && $1 == "-" { n++ } END { print n + 0 }' "$p.replay.yaml")
    line="$line replay=$st cost=$cost unrecorded=$unrec"
    local inside=0
    [ "$st" = ok ] && [ "$cost" -ge "$lo" ] && { [ -z "$hi" ] || [ "$cost" -lt "$hi" ]; } && inside=1
    if [ "$unrec" -eq 0 ] && [ $inside -eq 0 ]; then fail "$line: replay of a fully determined counterexample is outside the bucket"; fi
    if [ -n "$want_cost" ] && [ "$cost" != "$want_cost" ]; then fail "$line, expected cost $want_cost"; fi
    [ $inside -eq 1 ] && line="$line -> witness" || line="$line -> no witness"
  fi
  echo "$line"
}

# --- loop3: cost = 13n + 13, n in [0, 100000] -----------------------------------------------
CASE=loop3 HARNESS=tests/loop3/harness.c INCLUDES="" IR=$OUT/loop3.ll
$M instrument SRCS=tests/loop3/loop3.c TOP_FUNCTION=loop3 OUTPUT="$IR" REPORT="$OUT/loop3.report.yaml" ||
  fail "loop3: instrument"
weights=$(python3 -c 'import sys, yaml; print(" ".join(str(b["weight"]) for f in yaml.safe_load(open(sys.argv[1]))["functions"] for b in f["blocks"]))' "$OUT/loop3.report.yaml")
[ "$weights" = "7 4 5 4 2" ] || fail "loop3: block weights '$weights', expected '7 4 5 4 2'"
check b27 27 28 empty
check b52 52 53 reachable 52
check b53 53 65 empty
check b806 806 807 reachable 806

# --- calls: a helper also called before the top function, a callee reached by pointer ----------
CASE=calls HARNESS=tests/calls/harness.c INCLUDES="" IR=$OUT/calls.ll
$M instrument SRCS=tests/calls/calls.c TOP_FUNCTION=top OUTPUT="$IR" REPORT="$OUT/calls.report.yaml" ||
  fail "calls: instrument"
python3 - "$OUT/calls.report.yaml" <<'EOF' || fail "calls: instrumentation report"
import sys, yaml
r = yaml.safe_load(open(sys.argv[1]))
names = {f["name"] for f in r["functions"]}
assert {"top", "helper", "init", "twice"} <= names, names  # every defined function
assert r["indirect_calls"] == 1, r["indirect_calls"]
assert r["uncounted_calls"] == {"external": {}, "inline_asm": 0}, r["uncounted_calls"]
EOF
# Native costs of top(n) after init(), measured from the reset: the prover must agree with them.
clang-14 -m64 -O0 tests/calls/native.c "$IR" -o "$OUT/calls_native" &&
  "$OUT/calls_native" >"$OUT/calls.native.txt" || fail "calls: native oracle"
cost3=$(awk '$1 == 3 { print $2 }' "$OUT/calls.native.txt")
max=$(awk '$1 ~ /^[0-9]+$/ { print $2 }' "$OUT/calls.native.txt" | sort -n | tail -n 1)
echo "calls native costs (n cost):" $(awk '$1 ~ /^[0-9]+$/ { printf "%s:%s ", $1, $2 }' "$OUT/calls.native.txt")
check b3 "$cost3" "$((cost3 + 1))" reachable "$cost3"
check above "$((max + 1))" "" empty

# --- state_machine: one Simulink step, double inputs ------------------------------------------
CASE=state_machine HARNESS=tests/state_machine/harness.c INCLUDES=tests/state_machine/src
IR=$OUT/state_machine.ll
$M instrument SRCS="$(ls tests/state_machine/src/*.c | tr '\n' ' ')" TOP_FUNCTION=state_machine_step \
  INCLUDES="$INCLUDES" OUTPUT="$IR" REPORT="$OUT/state_machine.report.yaml" ||
  fail "state_machine: instrument"
# Sampling oracle: the `sample` target on pseudo-random rows (tests/state_machine/rows.py).
python3 tests/state_machine/rows.py "${SAMPLES:-20000}" "$OUT/state_machine.rows.tsv"
start=$(date +%s.%N)
$M sample HARNESS="$HARNESS" IR="$IR" INCLUDES="$INCLUDES" ROWS="$OUT/state_machine.rows.tsv" \
  OUTPUT="$OUT/state_machine.sample.yaml"
end=$(date +%s.%N)
python3 - "$OUT/state_machine.sample.yaml" >"$OUT/state_machine.sampled.txt" <<'EOF' ||
import collections, sys, yaml
r = yaml.safe_load(open(sys.argv[1]))
assert r["status"] == "ok", r["message"]
for cost, count in sorted(collections.Counter(x["cost"] for x in r["rows"] if x["status"] == "ok").items()):
    print(count, cost)
EOF
  fail "state_machine: sampling oracle (see $OUT/state_machine.sample.yaml)"
echo "state_machine sampled costs (count cost):" $(awk '{ printf "%s:%s ", $1, $2 }' "$OUT/state_machine.sampled.txt") \
  "in $(awk -v a="$start" -v b="$end" 'BEGIN { printf "%.1f", b - a }') s"

while read -r name lo hi want; do
  [ "$hi" = - ] && hi=""
  check "$name" "$lo" "$hi" "$want"
  hit=$(awk -v lo="$lo" -v hi="$hi" '$2 >= lo && (hi == "" || $2 < hi) { print $2 }' "$OUT/state_machine.sampled.txt" | head -n 1)
  if [ "$(field verdict "$OUT/state_machine.$name.prove.yaml")" = empty ]; then
    [ -z "$hit" ] || fail "state_machine [$lo,${hi:-inf}) proven empty, but sampling reached cost $hit"
  fi
  # The reachable buckets of buckets.txt were found by sampling: the rows must still reach them.
  [ "$want" != reachable ] || [ -n "$hit" ] || fail "state_machine [$lo,${hi:-inf}) not reached by sampling"
done <tests/state_machine/buckets.txt

echo
[ $failures -eq 0 ] && echo "ALL TESTS PASSED" || { echo "$failures FAILURE(S)"; exit 1; }
