// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040319-1.c
package main;

int
blah (int zzz)
{
  int foo;
  if (zzz >= 0)
    return 1;
  foo = (zzz >= 0 ? (zzz) : -(zzz));
  return foo;
}

int
main(void)
{
  if (blah (-1) != 1)
    return 1;
  else
    return 0;
}