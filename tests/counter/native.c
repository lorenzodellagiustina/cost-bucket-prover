/* Test oracle only: the cost of top(0), measured from the reset. */
#include <stdint.h>
#include <stdio.h>

extern int64_t __hux_cost;
int top(int x);

int main(void) {
  __hux_cost = 0;
  top(0);
  printf("%lld\n", (long long)__hux_cost);
  return 0;
}
