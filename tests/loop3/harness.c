#include "hux.h"

int loop3(int n);

int main(void) {
  int n;
  HUX_INPUT(int, "n", n);
  __VERIFIER_assume(n >= 0 && n <= 100000);
  HUX_START();
  loop3(n);
  HUX_CHECK_BUCKET();
  return 0;
}
