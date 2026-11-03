#!/bin/bash
# Builds HARNESS + IR natively with the counterexample CEX, runs it, and reports the measured cost
# and the input values that were actually executed.
#
# Inputs (environment): HARNESS, IR, CEX, OUTPUT (YAML); optional: INCLUDES, TIMEOUT (default 30).
#
# OUTPUT holds `status`, the `cost` (when ok), every recorded input as executed, and `unrecorded`:
# nondet types the counterexample left undetermined (they returned 0). `status` is:
#   ok               the program ran to the end and printed its cost: the only status that can give
#                    a witness (the cost must still lie in the bucket).
#   assume_violated  a __VERIFIER_assume failed: the counterexample's values are not valid inputs.
#   crash            the program exited non-zero, or without reaching HUX_CHECK_BUCKET().
#   timeout          the program did not finish within TIMEOUT.
#   build_error      the program could not be compiled or linked; the log has the messages.
# The script exits 0 for every status.
set -uo pipefail
: "${HARNESS:?}" "${IR:?}" "${CEX:?}" "${OUTPUT:?}"
PREFIX=${PREFIX:-/opt/cost-bucket-prover}
SEA_RT=${SEA_RT:-/home/usea/seahorn/lib/libsea-rt.a} # defines __seahorn_get_value_*
TIMEOUT=${TIMEOUT:-30}
LOG=${OUTPUT%.*}.log
EXE=${OUTPUT%.*}.exe

includes=("-I$PREFIX/include")
for dir in ${INCLUDES:-}; do includes+=("-I$dir"); done

# list KEY AWK_PROGRAM FILE
# Writes KEY as a YAML list. Each line of FILE (what the replayed program printed) that AWK_PROGRAM
# matches becomes one item.
list() {
  local items=""
  [ -n "$3" ] && items=$(awk "$2" "$3")
  if [ -n "$items" ]; then printf '%s:\n%s\n' "$1" "$items"; else echo "$1: []"; fi
}

# write STATUS [FILE]
# Writes the result YAML to OUTPUT: the status, the cost (from FILE when STATUS is ok, else null),
# the inputs and unrecorded nondet types printed in FILE, and the log path. FILE is the replayed
# program's stdout: it is omitted when the program did not run (build_error).
write() {
  {
    echo "status: $1"
    if [ "$1" = ok ]; then
      awk '$1 == "cost" { print "cost: " $2 }' "$2"
    else
      echo "cost: null"
    fi
    list inputs '$1 == "input" { printf "  - { name: \"%s\", kind: %s, value: \"%s\" }\n", $2, $3, $4 }' "${2:-}"
    list unrecorded '$1 == "unrecorded" { print "  - " $2 }' "${2:-}"
    echo "log: $LOG"
  } >"$OUTPUT"
}

if ! clang-14 -m64 -O0 -DHUX_REPLAY "${includes[@]}" -Wno-override-module \
  "$HARNESS" "$IR" "$CEX" "$PREFIX/runtime/replay_rt.c" "$SEA_RT" \
  -lstdc++ -lm -o "$EXE" >"$LOG" 2>&1; then
  write build_error
  exit 0
fi

out=${OUTPUT%.*}.stdout
timeout --kill-after=5 "$TIMEOUT" "$EXE" >"$out" 2>>"$LOG"
rc=$?
cat "$out" >>"$LOG"
if [ $rc -eq 124 ] || [ $rc -eq 137 ]; then
  write timeout "$out"
elif grep -q '^\[sea\]' "$out" "$LOG"; then
  write assume_violated "$out"
elif [ $rc -ne 0 ] || ! grep -q '^cost ' "$out"; then
  write crash "$out"
else
  write ok "$out"
fi
