#!/bin/bash
# Asks SeaHorn (Spacer) whether some input makes the cost of IR fall in [LO, HI).
#
# Inputs (environment): HARNESS (.c, see include/hux.h), IR (instrumented .ll), LO, OUTPUT (YAML);
#   optional: HI (unset = unbounded), INCLUDES, TIMEOUT (seconds, default 120),
#   CEX (counterexample harness .ll, written when the bucket is reachable).
#
# HARNESS is a C file with a `main` that:
# - assigns a non deterministic value to every input of the top function;
# - calls HUX_START() and the top function;
# - calls HUX_CHECK_BUCKET() with the bucket bounds LO and HI (SeaHorn will check whether the cost
#   is not in [LO, HI) and return a counterexample otherwise).
#
# OUTPUT holds the verdict: reachable | empty | unknown | timeout | error, plus timing and the log.
# The script exits 0 for every verdict; a non-zero exit means it was called wrongly.
set -uo pipefail
: "${HARNESS:?}" "${IR:?}" "${LO:?}" "${OUTPUT:?}"
PREFIX=${PREFIX:-/opt/cost-bucket-prover}
TIMEOUT=${TIMEOUT:-120}
LOG=${OUTPUT%.*}.log
CEX=${CEX:-${OUTPUT%.*}.cex.ll}

# SeaHorn keeps only the last -I flag, so include dirs are declared in the env.
C_INCLUDE_PATH=$PREFIX/include
for dir in ${INCLUDES:-}; do C_INCLUDE_PATH+=":$dir"; done
export C_INCLUDE_PATH

# Defines of the cost bounds.
defines=("-DHUX_LO=${LO}LL")
[ -n "${HI:-}" ] && defines+=("-DHUX_HI=${HI}LL")

# The solver (PROVE_SOLVER, default `sea`):
# - `sea`: `sea pf` with --crab. The Crab abstract interpreter computes numerical invariants (zones:
#   bounds on x - y) and passes them to Spacer, which alone often fails to find loop invariants such
#   as "cost - 13 i is constant" (and then times out on bounds of loops it does not unroll). Zones
#   cannot express "cost - 13 i is constant" either (coefficient 13).
# - `gspacer`: `sea smt` writes the Horn clauses, and a newer Z3 ($Z3_GSPACER) solves them with
#   Spacer's global guidance (`fp.spacer.global=true`), which finds such invariants by generalizing
#   across its lemmas: the bound of a 100000-iteration counting loop in under a second
#   (master-thesis/limitations/loop-invariants). Crab's invariants are not in the clauses `sea smt`
#   writes. Z3 alone does not give the counterexample harness, so on `sat` `sea pf` runs again, in
#   the time left, only to write it: `reachable` without a harness if it does not finish.
#   Global guidance can be much slower on deep reachable buckets (loop3's [806, 807): timeout,
#   where `sea` finds it in seconds).
# - `portfolio`: `sea` and `gspacer` at once, each with the whole TIMEOUT. `empty` from either, or
#   `reachable` from `sea` (with its harness), decides; `reachable` from `gspacer` alone waits for
#   `sea`'s harness. The log says which one decided. Twice the CPU.
SOLVER=${PROVE_SOLVER:-sea}
SMT=${OUTPUT%.*}.smt2
SEA_PF=(sea pf -m64 --inline --crab --crab-dom=zones "${defines[@]}" --cex="$CEX" "$HARNESS" "$IR")

last_answer() { grep -E '^(sat|unsat|unknown)$' "$1" 2>/dev/null | tail -n 1; }

# run_gspacer LOGFILE SECONDS: `sea smt`, then Z3 with global guidance, within SECONDS.
run_gspacer() {
  local log=$1 deadline=$(($(date +%s) + $2)) rc left
  : "${Z3_GSPACER:?the image sets it}"
  timeout --kill-after=5 "$2" sea smt -m64 --inline --step=large "${defines[@]}" \
    -o "$SMT" "$HARNESS" "$IR" >"$log" 2>&1
  rc=$?
  [ $rc -eq 0 ] || return $rc
  left=$((deadline - $(date +%s)))
  [ $left -gt 0 ] || return 124
  timeout --kill-after=5 "$left" "$Z3_GSPACER" fp.engine=spacer fp.spacer.global=true "$SMT" >>"$log" 2>&1
}

rm -f "$CEX"
answer=
start=$(date +%s.%N)
case "$SOLVER" in
  sea)
    timeout --kill-after=5 "$TIMEOUT" "${SEA_PF[@]}" >"$LOG" 2>&1
    rc=$?
    ;;
  gspacer)
    deadline=$(($(date +%s) + TIMEOUT))
    run_gspacer "$LOG" "$TIMEOUT"
    rc=$?
    if [ $rc -eq 0 ] && [ "$(last_answer "$LOG")" = sat ]; then
      left=$((deadline - $(date +%s)))
      echo "# sat: sea pf writes the counterexample harness (${left}s left)" >>"$LOG"
      [ $left -gt 0 ] && timeout --kill-after=5 "$left" "${SEA_PF[@]}" >"${LOG%.log}.cex.log" 2>&1
      answer=sat
    fi
    ;;
  portfolio)
    sea_log=${LOG%.log}.sea.log gs_log=${LOG%.log}.gspacer.log
    rm -f "$sea_log.rc" "$gs_log.rc"
    # Job control: each solver in its own process group, to stop the loser with all its children.
    set -m
    (timeout --kill-after=5 "$TIMEOUT" "${SEA_PF[@]}" >"$sea_log" 2>&1; echo $? >"$sea_log.rc") &
    sea_pid=$!
    (run_gspacer "$gs_log" "$TIMEOUT"; echo $? >"$gs_log.rc") &
    gs_pid=$!
    set +m
    while :; do
      if [ "$(last_answer "$gs_log")" = unsat ]; then winner=gspacer; break; fi
      if [ -s "$sea_log.rc" ] && [ "$(cat "$sea_log.rc")" = 0 ] &&
        [[ "$(last_answer "$sea_log")" =~ ^(sat|unsat)$ ]]; then winner=sea; break; fi
      if [ -s "$sea_log.rc" ] && [ -s "$gs_log.rc" ]; then winner=none; break; fi
      sleep 0.2
    done
    kill -- -"$sea_pid" -"$gs_pid" 2>/dev/null
    wait 2>/dev/null
    case "$winner" in
      gspacer) rc=0 answer=unsat ;;
      sea) rc=0 answer=$(last_answer "$sea_log") ;;
      *)
        if [ "$(last_answer "$gs_log")" = sat ]; then
          rc=0 answer=sat # `sea` did not finish: no harness
        else
          rc=$(cat "$sea_log.rc") answer=$(last_answer "$sea_log")
        fi
        ;;
    esac
    {
      echo "# portfolio: decided by $winner"
      echo "# --- sea pf (exit $(cat "$sea_log.rc" 2>/dev/null || echo killed))"
      cat "$sea_log"
      echo "# --- gspacer (exit $(cat "$gs_log.rc" 2>/dev/null || echo killed))"
      cat "$gs_log"
    } >"$LOG"
    rm -f "$sea_log" "$gs_log" "$sea_log.rc" "$gs_log.rc"
    ;;
  *)
    echo "prove: unknown PROVE_SOLVER '$SOLVER' (sea | gspacer | portfolio)" >&2
    exit 2
    ;;
esac
end=$(date +%s.%N)

answer=${answer:-$(last_answer "$LOG")}
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
