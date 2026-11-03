/* A loop with no closed form: at -O1 it keeps its phi nodes, before which no instruction may be
   inserted. */
int collatz(unsigned n) {
  int steps = 0;
  while (n > 1) {
    n = (n % 2) ? 3 * n + 1 : n / 2;
    steps++;
  }
  return steps;
}
