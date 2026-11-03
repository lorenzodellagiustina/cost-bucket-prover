#include "hux.h"

void init(void);
int top(int x);

int main(void) {
  init();
  int n = __VERIFIER_nondet_int();
  __VERIFIER_assume(n >= 0 && n <= 10);
  HUX_RECORD_INT("n", n);
  HUX_START();
  top(n);
  HUX_CHECK_BUCKET();
  return 0;
}
