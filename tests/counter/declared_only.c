/* The source only declares the counter (as include/hux.h does): the pass defines it. */
extern long long __hux_cost;

int top(int x) { return x + (int)(__hux_cost & 0); }
