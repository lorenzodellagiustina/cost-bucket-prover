#!/bin/bash
# Compiles SRCS to LLVM IR, links them, and inserts the cost counter in every function defined in
# them via the CostCounterPass. TOP_FUNCTION is the function whose cost will be measured. It does
# not limit the instrumentation: it only decides which calls the report lists as not counted (those
# that TOP_FUNCTION may execute).
#
# Inputs (environment): SRCS, TOP_FUNCTION, OUTPUT (instrumented .ll);
#   optional: INCLUDES, CFLAGS, WEIGHTS (YAML), REPORT (pass report, YAML).
set -euo pipefail
: "${SRCS:?}" "${TOP_FUNCTION:?}" "${OUTPUT:?}"
PREFIX=${PREFIX:-/opt/cost-bucket-prover}

# Work dir cleared on exit.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

includes=()
for dir in ${INCLUDES:-}; do includes+=("-I$dir"); done

# Compile and link to LLVM bitcode.
bcs=()
for src in $SRCS; do
  bc="$work/$(basename "$src").bc"
  clang-14 -c -emit-llvm -O0 -Xclang -disable-O0-optnone -g0 \
    "${includes[@]}" ${CFLAGS:-} "$src" -o "$bc"
  bcs+=("$bc")
done
llvm-link-14 "${bcs[@]}" -o "$work/linked.bc"

params="top=$TOP_FUNCTION"
[ -n "${WEIGHTS:-}" ] && params="$params;weights=$WEIGHTS"
[ -n "${REPORT:-}" ] && params="$params;report=$REPORT"
opt-14 -load-pass-plugin="$PREFIX/lib/libHuxCostCounter.so" \
  -passes="hux-cost-counter<$params>" -S "$work/linked.bc" -o "$OUTPUT"
