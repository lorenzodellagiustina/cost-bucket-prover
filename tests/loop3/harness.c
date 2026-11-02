#include "hux.h"

int loop3(int n);

int main(void) {
  int n = __VERIFIER_nondet_int();
  __VERIFIER_assume(n >= 0 && n <= 100000);
  HUX_RECORD_INT("n", n);
  loop3(n);
  HUX_CHECK_BUCKET();
  return 0;
}
