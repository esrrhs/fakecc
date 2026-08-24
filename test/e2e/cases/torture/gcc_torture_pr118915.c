// expect: 0
package main;

extern void abort(void);
void abort(void);

int a;

int
foo (int c, int d, int e, int f)
{
  if (!d || !e)
    return -22;
  if (c > 16)
    return -22;
  if (!f)
    return -22;
  return 2;
}

int
main ()
{
  if (foo (a + 21, a + 6, a + 34, a + 26) != -22)
    abort ();
  return 0;
}
