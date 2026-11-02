# cost-bucket-prover

Proves which execution-cost buckets a C/C++ function can reach. For each cost interval
`[lo, hi)`, the tool returns one of two answers:

- a proof that no input reaches the interval;
- an input whose execution does reach it, checked by running it natively.

The tool is a Docker image driven through `make`. Each make target is one command of the
performance-profiler-core External Service Executor, following the same convention as the
io-dependency-analyzer image.

## How it works

1. **`instrument`.** Compiles the sources to LLVM 14 IR and links them. The cost-counter pass
   (`pass/`) then adds `__hux_cost += w` at the start of every basic block of every function
   reachable from the top function. `w` is the sum of the weights of the block's instructions.
   After the top function returns, `__hux_cost` holds the cost of the executed path.
2. **`prove`.** Compiles a *harness* with the bucket bounds and asks SeaHorn (Spacer, a CHC
   solver) whether the error state `lo <= __hux_cost < hi` is reachable.
   - **unsat**: the bucket is **empty**, proven for every input admitted by the harness.
   - **sat**: the bucket is **reachable**. SeaHorn also writes a counterexample harness (`.ll`)
     with the values of the nondet calls.
3. **`replay`.** Builds the harness natively, links it with the counterexample, runs it, and
   prints the measured cost together with every input as executed. A reachable bucket only
   becomes a witness when the replayed cost actually lies in the bucket.

## Build

SeaHorn images exist only for amd64. On Apple Silicon the image runs under emulation.

```bash
docker build --platform linux/amd64 -t cost-bucket-prover:dev .
```

## Commands

The entrypoint is `make -f /opt/cost-bucket-prover/Makefile`, so a command is a target plus
variables. All paths are container paths, relative to the working directory `/data`.

| Target | Required | Optional | Produces |
|---|---|---|---|
| `instrument` | `SRCS`, `TOP_FUNCTION`, `OUTPUT` | `INCLUDES`, `CFLAGS`, `WEIGHTS`, `REPORT` | instrumented `.ll`; JSON block report |
| `prove` | `HARNESS`, `IR`, `LO`, `OUTPUT` | `HI` (unset means unbounded above), `INCLUDES`, `TIMEOUT` (120 s), `CEX` | YAML verdict; counterexample `.ll`; log |
| `replay` | `HARNESS`, `IR`, `CEX`, `OUTPUT` | `INCLUDES`, `TIMEOUT` (30 s) | YAML with cost and inputs; log |

Example:

```bash
docker run --rm --platform linux/amd64 -v "$PWD":/data cost-bucket-prover:dev \
  prove HARNESS=harness.c IR=out.ll LO=52 HI=53 OUTPUT=b52.yaml
```

`prove` and `replay` exit with 0 for every verdict or status. A non-zero exit code means the
command was called wrongly or the tool failed.

### Weights

The `WEIGHTS` file maps LLVM opcode names to weights:

```json
{ "default": 1, "opcodes": { "load": 2, "store": 2, "call": 4 } }
```

Without a file, every instruction weighs 1. Debug intrinsics always weigh 0.

### Instrumentation report (`REPORT`)

The report lists, for each instrumented function, its blocks and their weights. Under
`uncounted_calls` it lists the calls whose callee is **not** counted: functions that are only
declared (`external`, with the number of call sites), `indirect` calls, and `inline_asm`.

### Harness contract (`include/hux.h`)

The harness is a C file with a `main` that:

1. assigns every input of the top function from `__VERIFIER_nondet_<type>()`, optionally
   constrained with `__VERIFIER_assume`;
2. records each input with `HUX_RECORD_INT`, `HUX_RECORD_UINT` or `HUX_RECORD_FLOAT`
   (`name`, `value`);
3. calls the top function;
4. ends with `HUX_CHECK_BUCKET()`.

The same file serves both `prove` and `replay`: `replay` compiles it with `-DHUX_REPLAY`. See
`tests/*/harness.c`.

### `prove` output

```yaml
verdict: reachable      # reachable | empty | unknown | timeout | error
lo: 52
hi: 85                  # null when unbounded
seconds: 2.051
exit_code: 0
cex: path/to/b52.cex.ll # null unless reachable
log: path/to/b52.log
```

### `replay` output

```yaml
status: ok              # ok | assume_violated | crash | timeout | build_error
cost: 8                 # null unless ok
inputs:                 # every recorded input, as executed
  - { name: "state_machine_U.brake", kind: float, value: "0" }
unrecorded:             # nondet types the counterexample did not determine (they returned 0)
  - double
log: path/to/b8.replay.log
```

## Known limitations

These are reported, not worked around.

- **Floating point.** SeaHorn treats floating-point operations and comparisons as
  non-deterministic. That keeps `empty` verdicts sound but makes some `reachable` verdicts
  spurious. Its counterexamples never contain floating-point values, which then default to 0
  during replay and appear under `unrecorded`. In practice, a bucket whose cost depends on float
  inputs gets a `reachable` verdict whose replay usually lands outside the bucket. Such a bucket
  is undecided.
- **Integers.** SeaHorn encodes integers as mathematical integers, so wraparound is not modeled
  (seahorn/seahorn#537). The replay catches spurious witnesses. An `empty` verdict on code that
  really overflows may be wrong.
- **LLVM semantics.** SeaHorn optimizes the IR and may exploit undefined behavior (for example
  `nsw`). Verdicts hold for that semantics.
- **Depth.** Witnesses for long paths time out: on `loop3`, around 600 loop iterations. Proving
  a bucket empty above the maximum also times out when the loop bound is large (100,000 on
  `loop3`).
- **Uncounted code.** The cost of callees listed under `uncounted_calls` is not counted.
- **Weights.** The default weight is "one per instruction". Weights come from a hand-written file
  and are not taken from the platform's cost model.

## Tests

```bash
docker run --rm --platform linux/amd64 -v "$PWD/tests":/data/tests \
  --entrypoint bash cost-bucket-prover:dev tests/run.sh
```

- **`loop3`** (cost `13n + 13`): checks exact block weights, exact verdicts, and that each
  witness replays to the expected cost.
- **`state_machine`** (Simulink-generated code): runs one step of the model. Its inputs are the
  `real_T` inports plus the chart state. For every bucket the suite checks:
  - a fully determined counterexample must replay inside its bucket;
  - no cost reached by native random sampling (`sample.c`, including ±inf and NaN) may lie in a
    bucket proven `empty`.
