#!/bin/bash
# End-to-end tests of the image's make interface. Run from the repository root:
#   docker run --rm --platform linux/amd64 -v "$PWD/tests":/data/tests \
#     --entrypoint bash cost-bucket-prover:dev tests/run.sh
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
$M instrument SRCS=tests/loop3/loop3.c TOP_FUNCTION=loop3 OUTPUT="$IR" REPORT="$OUT/loop3.report.json" ||
  fail "loop3: instrument"
weights=$(grep -o '"weight": [0-9]*' "$OUT/loop3.report.json" | awk '{ printf "%s ", $2 }')
[ "$weights" = "7 4 5 4 2 " ] || fail "loop3: block weights '$weights', expected '7 4 5 4 2 '"
check b27 27 28 empty
check b52 52 53 reachable 52
check b53 53 65 empty
check b806 806 807 reachable 806

# --- state_machine: one Simulink step, double inputs ------------------------------------------
CASE=state_machine HARNESS=tests/state_machine/harness.c INCLUDES=tests/state_machine/src
IR=$OUT/state_machine.ll
$M instrument SRCS="$(ls tests/state_machine/src/*.c | tr '\n' ' ')" TOP_FUNCTION=state_machine_step \
  INCLUDES="$INCLUDES" OUTPUT="$IR" REPORT="$OUT/state_machine.report.json" ||
  fail "state_machine: instrument"
clang-14 -m64 -O0 -Itests/state_machine/src tests/state_machine/sample.c "$IR" -lm -o "$OUT/sample" &&
  "$OUT/sample" 20000 | sort -n | uniq -c >"$OUT/state_machine.sampled.txt" ||
  fail "state_machine: sampling oracle"
echo "state_machine sampled costs (count cost):" $(awk '{ printf "%s:%s ", $1, $2 }' "$OUT/state_machine.sampled.txt")

while read -r name lo hi want; do
  [ "$hi" = - ] && hi=""
  check "$name" "$lo" "$hi" "$want"
  if [ "$(field verdict "$OUT/state_machine.$name.prove.yaml")" = empty ]; then
    hit=$(awk -v lo="$lo" -v hi="$hi" '$2 >= lo && (hi == "" || $2 < hi) { print $2 }' "$OUT/state_machine.sampled.txt" | head -n 1)
    [ -z "$hit" ] || fail "state_machine [$lo,${hi:-inf}) proven empty, but sampling reached cost $hit"
  fi
done <tests/state_machine/buckets.txt

echo
[ $failures -eq 0 ] && echo "ALL TESTS PASSED" || { echo "$failures FAILURE(S)"; exit 1; }
