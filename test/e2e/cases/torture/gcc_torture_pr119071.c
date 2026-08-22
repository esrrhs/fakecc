// expect: 0
package main;

extern void abort(void);
void abort(void);

int a, b;

int
main ()
{
  int c = 0;
  if (a + 2)
    c = 1;
  int d = (1 + c - 2 + c == 1) - 1;
  b = ((d + 1) << d) + d;
  if (b != 1)
    abort ();
  return 0;
}
