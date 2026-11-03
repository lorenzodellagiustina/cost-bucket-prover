#!/bin/bash
# Compiles SRCS to LLVM IR, links them, and inserts the cost counter in every function reachable
# from TOP_FUNCTION.
#
# Inputs (environment): SRCS, TOP_FUNCTION, OUTPUT (instrumented .ll);
#   optional: INCLUDES, CFLAGS, WEIGHTS (YAML), REPORT (pass report, YAML).
set -euo pipefail
: "${SRCS:?}" "${TOP_FUNCTION:?}" "${OUTPUT:?}"
PREFIX=${PREFIX:-/opt/cost-bucket-prover}

work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT

includes=()
for dir in ${INCLUDES:-}; do includes+=("-I$dir"); done

# -O0 without optnone: the IR keeps the source structure, and SeaHorn may still optimize it.
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
