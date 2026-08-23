/* PR tree-optimization/82387 */

// expect: 0
package main;

struct A { int b; };
int f = 1;

struct A
foo (void)
{
  struct A h[80];
  int i;
  for (i = 0; i < 80; i++) h[i].b = 1;
  return h[24];
}

int
main (void)
{
  struct A i = foo (), j = i;
  j.b && (f = 0);
  return f; 
}
