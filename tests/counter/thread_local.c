/* A thread-local counter is another variable per thread: the pass must refuse it. */
_Thread_local long long __hux_cost = 0;

int top(int x) { return x + 1; }
