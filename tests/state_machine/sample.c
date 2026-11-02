/* Test oracle only: runs the instrumented step on N pseudo-random inputs and prints each cost.
   Used to cross-check `empty` verdicts (a sampled cost inside an empty bucket is a bug). */
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "state_machine.h"

extern int64_t __hux_cost;

static double pick(void) {
  static const double special[] = {0.0, 1.0, 2.0, 100.0, -1.0, 0.5, -50.0, 1e3, INFINITY, -INFINITY};
  switch (rand() % 4) {
  case 0: return special[rand() % 10];
  case 1: return NAN;
  default: return ((double)rand() / RAND_MAX - 0.5) * 2e4;
  }
}

int main(int argc, char **argv) {
  int n = argc > 1 ? atoi(argv[1]) : 20000;
  srand(1);
  for (int k = 0; k < n; ++k) {
    state_machine_U.brake = rand() % 2 ? 100.0 : pick();
    state_machine_U.accellerator = rand() % 2 ? 0.0 : pick();
    state_machine_U.button = (double)(rand() % 3);
    state_machine_U.shutdown_closed = rand() % 2 ? 1.0 : pick();
    for (int i = 0; i < 3; ++i) state_machine_U.battery_voltage[i] = pick();
    state_machine_DW.is_active_c3_state_machine = rand() % 2;
    state_machine_DW.is_c3_state_machine = 1 + rand() % 4;
    __hux_cost = 0;
    state_machine_step();
    printf("%lld\n", (long long)__hux_cost);
  }
  return 0;
}
