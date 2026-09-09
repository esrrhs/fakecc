// expect: 0
package main;

/* PR40657 */

extern void abort (void);
extern void exit (int);

long long v = 0x123456789abc;

void bar (int *x)
{
}

long long foo (void)
{
  int x;
  bar (&x);
  return v;
}

int main (void)
{
  if (foo () != v)
    abort ();
  exit (0);
}
