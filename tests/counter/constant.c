/* A constant counter cannot be incremented: the pass must refuse it. */
const long long __hux_cost = 0;

int top(int x) { return x + (int)__hux_cost; }
