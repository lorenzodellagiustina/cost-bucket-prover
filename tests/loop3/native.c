/* Test oracle only: the cost of loop3(n), measured from the reset, for a few n. */
#include <stdint.h>
#include <stdio.h>

extern int64_t __hux_cost;
int loop3(int n);

int main(void) {
  static const int ns[] = {0, 1, 3, 61, 1000};
  for (unsigned k = 0; k < sizeof ns / sizeof ns[0]; ++k) {
    __hux_cost = 0;
    loop3(ns[k]);
    printf("%d %lld\n", ns[k], (long long)__hux_cost);
  }
  return 0;
}
