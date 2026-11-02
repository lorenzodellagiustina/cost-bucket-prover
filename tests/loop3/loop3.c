/* spike: input-dependent trip count; cost is affine in n */
int loop3(int n) {
  int s = 0;
  for (int i = 0; i < n; i++) {
    s += i;
  }
  return s;
}
