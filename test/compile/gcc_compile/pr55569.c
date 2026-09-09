package main;
int foo(int x) {
  void *p = x ? (void *)1 : (void *)0;
  long b = (long)p;
  if (b) return 0;
  return 1;
}
int main() { return 0; }
