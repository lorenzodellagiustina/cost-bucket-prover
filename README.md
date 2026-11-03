# cost-bucket-prover

Proves which execution-cost buckets a C/C++ function can reach. For each cost interval
`[lo, hi)`, the tool returns one of two answers:

- a proof that no input reaches the interval;
- an input whose execution does reach it, checked by running it natively.

The tool is a Docker image driven through `make`.

## How it works

1. **`instrument`.** Compiles the sources to LLVM 14 IR and links them. The cost-counter pass
   (`pass/`) then adds `__hux_cost += w` at the start of every basic block of every function
   defined in the sources. `w` is the sum of the weights of the block's instructions. The harness
   (read the point below) resets the counter right before calling the top function, so after it
   returns `__hux_cost` holds the cost of that call's executed path, whatever the harness ran
   before.
2. **`prove`.** Compiles a *harness* with the bucket bounds and asks SeaHorn (Spacer, a CHC solver)
   whether the error state `lo <= __hux_cost < hi` is reachable.
   - **unsat**: the bucket is **empty**, proven for every input admitted by the harness.
   - **sat**: the bucket is **reachable**. SeaHorn also writes a counterexample harness (`.ll`) with
     the values of the nondet calls.
3. **`replay`.** Builds the harness natively, links it with the counterexample, runs it, and prints
   the measured cost together with every input as executed. A reachable bucket only becomes a
   witness when the replayed cost actually lies in the bucket.

## Build

To build the image, run in the repo root:

```bash
docker build -t cost-bucket-prover:$(git rev-parse HEAD) .
```

The image is tagged with the hash of the commit it is built from (`<ref>` below).

> [!NOTE]
> SeaHorn images exist only for amd64. On Apple Silicon build the image for amd64:
>
> ```bash
> docker build --platform linux/amd64 -t cost-bucket-prover:$(git rev-parse --short=9 HEAD) .
> ```
>
> Then the image can be run on Apple Silicon through emulation.


## Usage

The entrypoint is `make -f /opt/cost-bucket-prover/Makefile`, so a command is a target plus
variables. All paths are container paths, relative to the working directory `/data`.

| Target | Required | Optional | Produces |
|---|---|---|---|
| `instrument` | `SRCS`, `TOP_FUNCTION`, `OUTPUT` | `INCLUDES`, `CFLAGS`, `WEIGHTS`, `REPORT` | instrumented `.ll`; YAML block report |
| `prove` | `HARNESS`, `IR`, `LO`, `OUTPUT` | `HI` (unset means unbounded above), `INCLUDES`, `TIMEOUT` (120 s), `CEX` | YAML verdict; counterexample `.ll`; log |
| `replay` | `HARNESS`, `IR`, `CEX`, `OUTPUT` | `INCLUDES`, `TIMEOUT` (30 s) | YAML with cost and inputs; log |

Example:

```bash
docker run --rm --platform linux/amd64 -v "$PWD":/data cost-bucket-prover:<ref> \
  prove HARNESS=harness.c IR=out.ll LO=52 HI=53 OUTPUT=b52.yaml
```

`prove` and `replay` exit with 0 for every verdict or status. A non-zero exit code means the
command was called wrongly or the tool failed.

### Weights

The `WEIGHTS` file maps LLVM opcode names to weights:

```yaml
default: 1
opcodes:
  load: 2
  store: 2
  call: 4
```

Without a file, every instruction weighs 1. Debug intrinsics always weigh 0. An unknown key or a
value that is not a non-negative integer is an error.

### Instrumentation report (`REPORT`)

The report lists, for each instrumented function, its blocks and their weights. `uncounted_calls`
lists the calls that the top function may execute whose callee has no code in the sources, so its
cost is **not** counted.

### Harness (`include/hux.h`)

The harness is a C file with a `main` that:

1. assigns every input of the top function from `__VERIFIER_nondet_<type>()`, optionally constrained
   with `__VERIFIER_assume`;
2. records each input (`name`, `value`), necessary for replaying a counterexample;
3. calls `HUX_START()` right before the top function to reset the counter (helpful in case other
   functions are called before it), then calls it;
4. ends with `HUX_CHECK_BUCKET()`.

The same file serves both `prove` and `replay`: `replay` compiles it with `-DHUX_REPLAY`. See
`tests/*/harness.c`.

TODO: add an automatic harness generator.

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

- **Floating point.** SeaHorn treats floating-point operations and comparisons as non-deterministic.
  That keeps `empty` verdicts sound but makes some `reachable` verdicts spurious. Its
  counterexamples never contain floating-point values, which then default to 0 during replay and
  appear under `unrecorded`. In practice, a bucket whose cost depends on float inputs gets a
  `reachable` verdict whose replay usually lands outside the bucket. Such a bucket is undecided.
- **Integers.** SeaHorn encodes integers as mathematical integers, so wraparound is not modeled
  (seahorn/seahorn#550). The replay catches spurious witnesses. An `empty` verdict on code that
  really overflows may be wrong.
- **LLVM semantics.** SeaHorn optimizes the IR and may exploit undefined behavior (for example
  `nsw`). Verdicts hold for that semantics.
- **Depth.** Witnesses for long paths time out: on `loop3`, around 600 loop iterations. Proving
  a bucket empty above the maximum also times out when the loop bound is large (100,000 on
  `loop3`).
- **Uncounted code.** The cost of callees listed under `uncounted_calls` is not counted.
- **Weights.** The default weight is "one per instruction". Weights come from a hand-written file
  and are not taken from the platform's cost model.

<!--
## Development

The repo contains a dev container supposed to be used with VS Code's Dev Containers extension.

### The counter variable

The pass creates the global `__hux_cost` (an `i64`, initially 0). If the module already has one,
for example because a source includes `include/hux.h`, which declares it, the pass uses it and
prints a warning. It refuses, with an error, a counter that is not an `i64`, that is constant (a
store to a constant is undefined, and an optimizer may fold the counter to its initializer) or that
is thread-local (the harness would read another thread's copy). A second variable with the same
name would otherwise silently get another name, and the harness would read a counter that is never
incremented.

The warning also says that a counter declared differently elsewhere (another type, size or
linkage) may cause undefined behavior. The pass only sees the module it is given, so it cannot check
that.

The pass (`pass/`) is split by responsibility:

| File | Role |
|---|---|
| `Plugin.cpp` | Registers the pass with the new pass manager and parses `<top=…;weights=…;report=…>`. |
| `CostCounterPass.h/.cpp` | The instrumentation: computes each block's weight and inserts the increments. |
| `Weights.h/.cpp` | The per-opcode weights, read from the YAML `WEIGHTS` file. |
| `CallAnalysis.h/.cpp` | The calls the top function may execute whose cost is not counted. |
| `Report.h/.cpp` | The YAML report. |

The pass alone is tested without SeaHorn, in seconds, from the dev container:

```bash
tests/run_pass.sh
```

It builds the pass, instruments the test programs, runs them natively, and checks the block weights
and the costs:

- **`loop3`** at `-O0`: the exact block weights and a cost of `13n + 13`.
- **`collatz`** at `-O1`: its blocks start with `phi` nodes, after which the increments must go,
  and its cost is affine in the number of loop steps.
- **`calls`**: every defined function is instrumented, including the callee reached only through
  a pointer.

The end-to-end tests below need the SeaHorn image. From the dev container, they run through
Docker outside of Docker: the daemon is the host's, so the bind mount takes the host's path,
`$LOCAL_WORKSPACE_FOLDER/tests`.

## Tests

```bash
docker run --rm --platform linux/amd64 -v "$PWD/tests":/data/tests \
  --entrypoint bash cost-bucket-prover:<ref> tests/run.sh
```

- **`loop3`** (cost `13n + 13`): checks exact block weights, exact verdicts, and that each
  witness replays to the expected cost.
- **`calls`**: a helper that the harness also calls before the top function (through `init`), and
  a callee reached only through a function pointer. Checks that every defined function is
  instrumented, that the indirect call is reported, and that the prover's witness has exactly the
  native cost of the top function's call alone.
- **`state_machine`** (Simulink-generated code): runs one step of the model. Its inputs are the
  `real_T` inports plus the chart state. For every bucket the suite checks:
  - a fully determined counterexample must replay inside its bucket;
  - no cost reached by native random sampling (`sample.c`, including ±inf and NaN) may lie in a
    bucket proven `empty`.
-->
