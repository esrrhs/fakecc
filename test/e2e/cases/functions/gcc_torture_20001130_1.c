// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001130-1.c
package main;

static inline int bar(void) { return 1; }
static int mem[3];

static int foo(int x)
{
  if (x != 0)
    return x;

  mem[x++] = foo(bar());

  if (x != 1)
    return 1;

  return 0;
}

int main()
{
  foo(0);
  return 0;
}