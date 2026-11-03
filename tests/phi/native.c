/* Test oracle only: for a few n, the cost of collatz(n) and its number of steps ("n cost steps"). */
#include <stdint.h>
#include <stdio.h>

extern int64_t __hux_cost;
int collatz(unsigned n);

int main(void) {
  static const unsigned ns[] = {1, 2, 3, 6, 7, 27};
  for (unsigned k = 0; k < sizeof ns / sizeof ns[0]; ++k) {
    __hux_cost = 0;
    int steps = collatz(ns[k]);
    printf("%u %lld %d\n", ns[k], (long long)__hux_cost, steps);
  }
  return 0;
}
