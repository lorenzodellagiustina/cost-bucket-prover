"""Writes N pseudo-random rows of state_machine inputs (the `sample` target's rows format).

Test oracle only: the costs of these rows cross-check the prover's `empty` verdicts (a sampled cost
inside an empty bucket is a bug). Any distribution serves that purpose: this one favours values the
chart compares against, plus infinities and NaN, to reach many costs. Usage: python3 rows.py N OUT.tsv
"""
import math
import random
import sys

COLUMNS = [
    "state_machine_U.brake",
    "state_machine_U.accellerator",
    "state_machine_U.button",
    "state_machine_U.shutdown_closed",
    "state_machine_U.battery_voltage[0]",
    "state_machine_U.battery_voltage[1]",
    "state_machine_U.battery_voltage[2]",
    "state_machine_DW.is_active_c3_state_machine",
    "state_machine_DW.is_c3_state_machine",
]
SPECIAL = [0.0, 1.0, 2.0, 100.0, -1.0, 0.5, -50.0, 1e3, math.inf, -math.inf]


def pick(rng):
    k = rng.randrange(4)
    if k == 0:
        return rng.choice(SPECIAL)
    if k == 1:
        return math.nan
    return (rng.random() - 0.5) * 2e4


def state(rng):
    """A chart-state byte: any of the 256 values, half of the time one of 0..5 (the chart's states
    1..4 and their neighbours), which uniform draws would rarely give."""
    return rng.randrange(6) if rng.randrange(2) else rng.randrange(256)


def main():
    n, path = int(sys.argv[1]), sys.argv[2]
    rng = random.Random(1)
    with open(path, "w") as out:
        out.write("\t".join(COLUMNS) + "\n")
        for _ in range(n):
            row = [
                100.0 if rng.randrange(2) else pick(rng),
                0.0 if rng.randrange(2) else pick(rng),
                float(rng.randrange(3)),
                1.0 if rng.randrange(2) else pick(rng),
                pick(rng),
                pick(rng),
                pick(rng),
            ]
            # repr() of a float reads back exactly with strtod.
            values = [repr(v) for v in row] + [str(state(rng)), str(state(rng))]
            out.write("\t".join(values) + "\n")


if __name__ == "__main__":
    main()
