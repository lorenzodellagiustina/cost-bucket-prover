/* Calls the cost counter must handle:
   - `helper` is called both by `init`, which the harness calls before the top function, and by the
     top function: only the top function's call may be counted;
   - `twice` is called only through a function pointer: its cost must still be counted. */
int helper(int x) {
  int s = 0;
  for (int i = 0; i < x; i++) s += i;
  return s;
}

static int twice(int x) { return 2 * x; }

int (*op)(int) = twice;

void init(void) { helper(10); }

int top(int x) { return helper(x) + op(x); }
