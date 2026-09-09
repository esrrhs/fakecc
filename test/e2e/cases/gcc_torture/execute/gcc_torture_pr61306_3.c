// expect: 0
package main;

/* PR tree-optimization/61306 */

extern void abort (void);

short a = -1;
int b;
char c;

int
main (void)
{
  c = a;
  b = a | c;
  if (b != -1)
    abort ();
  return 0;
}
