#!/bin/bash
# Asks SeaHorn (Spacer) whether some input makes the cost of IR fall in [LO, HI).
#
# Inputs (environment): HARNESS (.c, see include/hux.h), IR (instrumented .ll), LO, OUTPUT (YAML);
#   optional: HI (unset = unbounded), INCLUDES, TIMEOUT (seconds, default 120),
#   CEX (counterexample harness .ll, written when the bucket is reachable).
#
# OUTPUT holds `verdict`: reachable | empty | unknown | timeout | error, plus timing and the log.
# The script exits 0 for every verdict; a non-zero exit means it was called wrongly.
set -uo pipefail
: "${HARNESS:?}" "${IR:?}" "${LO:?}" "${OUTPUT:?}"
PREFIX=${PREFIX:-/opt/cost-bucket-prover}
TIMEOUT=${TIMEOUT:-120}
LOG=${OUTPUT%.*}.log
CEX=${CEX:-${OUTPUT%.*}.cex.ll}

# `sea` keeps only the last -I flag, so include directories go through the environment.
C_INCLUDE_PATH=$PREFIX/include
for dir in ${INCLUDES:-}; do C_INCLUDE_PATH+=":$dir"; done
export C_INCLUDE_PATH
defines=("-DHUX_LO=${LO}LL")
[ -n "${HI:-}" ] && defines+=("-DHUX_HI=${HI}LL")

rm -f "$CEX"
start=$(date +%s.%N)
timeout --kill-after=5 "$TIMEOUT" sea pf -m64 --inline "${defines[@]}" \
  --cex="$CEX" "$HARNESS" "$IR" >"$LOG" 2>&1
rc=$?
end=$(date +%s.%N)

answer=$(grep -E '^(sat|unsat|unknown)$' "$LOG" | tail -n 1)
if [ $rc -eq 124 ] || [ $rc -eq 137 ]; then
  verdict=timeout
elif [ $rc -ne 0 ]; then
  verdict=error
else
  case "$answer" in
    sat) verdict=reachable ;;
    unsat) verdict=empty ;;
    unknown) verdict=unknown ;;
    *) verdict=error ;;
  esac
fi
cex=null
[ "$verdict" = reachable ] && [ -s "$CEX" ] && cex="$CEX"

{
  echo "verdict: $verdict"
  echo "lo: $LO"
  echo "hi: ${HI:-null}"
  echo "seconds: $(awk -v a="$start" -v b="$end" 'BEGIN { printf "%.3f", b - a }')"
  echo "exit_code: $rc"
  echo "cex: $cex"
  echo "log: $LOG"
} >"$OUTPUT"
