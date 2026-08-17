// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20040313-1.c
package main;

/* PR middle-end/14470 */
/* Origin: Lodewijk Voge <lvoge@cs.vu.nl> */

int main()
{
  int t[1025] = { 1024 }, d;

  d = 0;
  d = t[d]++;
  if (t[0] != 1025)
    return 1;
  if (d != 1024)
    return 1;
  return 0;
}