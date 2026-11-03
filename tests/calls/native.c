/* Test oracle only: for each n, the cost of top(n) after init(), measured from the reset; then the
   cost of the function `op` points to, called alone ("op COST"). */
#include <stdint.h>
#include <stdio.h>

extern int64_t __hux_cost;
extern int (*op)(int);
void init(void);
int top(int x);

int main(void) {
  for (int n = 0; n <= 10; ++n) {
    init();
    __hux_cost = 0;
    top(n);
    printf("%d %lld\n", n, (long long)__hux_cost);
  }
  __hux_cost = 0;
  op(5);
  printf("op %lld\n", (long long)__hux_cost);
  return 0;
}
