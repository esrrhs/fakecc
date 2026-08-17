// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000622-1.c
package main;

long foo(long a, long b, long c)
{
  if (a != 12 || b != 1 || c != 11)
    return 1;
  return 0;
}
long bar (long a, long b)
{
  return b;
}
void baz (long a, long b, void *c)
{
  long d;
  d = (long)c;
  foo(d, bar (a, 1), b);
}
int main()
{
  baz (10, 11, (void *)12);
  return 0;
}