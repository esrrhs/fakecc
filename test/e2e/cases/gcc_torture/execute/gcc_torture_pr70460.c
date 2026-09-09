// expect: 0
package main;

/* PR rtl-optimization/70460 */

extern void abort(void);

int c;

void
foo (int x)
{
  static int b[2];
  b[0] = &&lab1 - &&lab0;
  b[1] = &&lab2 - &&lab0;
  void *a = (char *)&&lab0 + b[x];
  goto *a;
lab1:
  c += 2;
lab2:
  c++;
lab0:
  ;
}

int
main ()
{
  foo (0);
  if (c != 3)
    abort ();
  foo (1);
  if (c != 4)
    abort ();
  return 0;
}
