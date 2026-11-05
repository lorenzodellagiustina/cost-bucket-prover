#!/bin/bash
# Runs HARNESS + IR natively once per row of ROWS, each row in a fresh process, and reports the
# measured cost of each row.
#
# Inputs (environment): HARNESS, IR, ROWS (see runtime/sample_rt.c), OUTPUT (YAML);
#   optional: INCLUDES, TIMEOUT (seconds per row, default 10), PRINT_INPUTS (1: list the inputs of
#   each ok row as executed), EXE (path of the sampler executable: built if it does not exist, so
#   that several calls on the same HARNESS and IR build it once).
#
# OUTPUT holds `status` (ok | input_error | build_error), `message`, and `rows`: one entry per row
# run, `{status, cost}` with status ok | rejected | crash | timeout (see runtime/sample_rt.c). The
# script exits 0 for every status.
set -uo pipefail
: "${HARNESS:?}" "${IR:?}" "${ROWS:?}" "${OUTPUT:?}"
PREFIX=${PREFIX:-/opt/cost-bucket-prover}
TIMEOUT=${TIMEOUT:-10}
LOG=${OUTPUT%.*}.log
EXE=${EXE:-${OUTPUT%.*}.exe}
case "$EXE" in */*) ;; *) EXE=./$EXE ;; esac # run it as a path, not looked up in PATH

includes=("-I$PREFIX/include")
for dir in ${INCLUDES:-}; do includes+=("-I$dir"); done

: >"$LOG"
if [ ! -x "$EXE" ] && ! clang-14 -O0 -DHUX_SAMPLE "${includes[@]}" -Wno-override-module \
  "$HARNESS" "$IR" "$PREFIX/runtime/sample_rt.c" -lstdc++ -lm -o "$EXE" >>"$LOG" 2>&1; then
  printf 'status: build_error\nmessage: "see %s"\nrows: []\n' "$LOG" >"$OUTPUT"
  exit 0
fi

"$EXE" "$ROWS" "$OUTPUT" "$TIMEOUT" "${PRINT_INPUTS:-0}" 2>>"$LOG" ||
  printf 'status: build_error\nmessage: "the sampler failed, see %s"\nrows: []\n' "$LOG" >"$OUTPUT"
exit 0
