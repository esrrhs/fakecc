// expect: 0
package main;

/* PR tree-optimization/78586 */

extern void abort (void);
extern int sprintf (char *, const char *, ...);

void
foo (unsigned long x)
{
  char a[30];
  unsigned long b = sprintf (a, "%lu", x);
  if (b != 4)
    abort ();
}

int
main (void)
{
  foo (1000);
  return 0;
}
