/* PR tree-optimization/82388 */

// expect: 0
package main;

struct A { int b; int c; int d; } e;

struct A
foo (void)
{
  struct A h[30];
  int i;
  for (i = 0; i < 30; i++) { h[i].b = 0; h[i].c = 0; h[i].d = 0; }
  return h[29]; 
}

int
main (void)
{
  e = foo ();
  return e.b; 
}
