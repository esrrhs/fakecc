// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20080424-1.c
package main;

/* PR tree-optimization/36008 */

int g[48][3][3];

int 
bar (int x[3][3], int y[3][3])
{
  static int i;
  if (x != g[i + 8] || y != g[i++])
    return 1;

    return 0;}

static inline void __attribute__ ((always_inline))
foo (int x[][3][3])
{
  int i;
  for (i = 0; i < 8; i++)
    {
      int k = i + 8;
      bar (x[k], x[k - 8]);
    }
}

int
main ()
{
  foo (g);
  return 0;
}