// expect: 0
package main;

/* PR tree-optimization/84235 */

extern void abort(void);

int
main ()
{
  double d = 1.0 / 0.0;
  _Bool b = d == d && (d - d) != (d - d);
  if (!b)
    abort ();
  return 0;
}
