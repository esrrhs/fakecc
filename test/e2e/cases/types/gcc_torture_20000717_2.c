// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000717-2.c
package main;

static int
compare (long long foo)
{
  if (foo < 4294967297LL)
    return 1;

    return 0;}
int main(void)
{
  compare (8589934591LL);
  return 0;
}