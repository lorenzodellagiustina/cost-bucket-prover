#include "hux.h"

void init(void);
int top(int x);

int main(void) {
  init();
  int n;
  HUX_INPUT(int, "n", n);
  __VERIFIER_assume(n >= 0 && n <= 10);
  HUX_START();
  top(n);
  HUX_CHECK_BUCKET();
  return 0;
}
