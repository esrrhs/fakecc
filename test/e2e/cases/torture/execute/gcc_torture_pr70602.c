/* PR tree-optimization/70602 */

// expect: 0
package main;

struct __attribute__((packed)) S
{
  int s : 1;
  int t : 20;
};

int a, b, c;

int
main (void)
{
  for (; a < 1; a++)
    {
      struct S e[21];
      int i;
      for (i = 0; i < 21; i++) {
        e[i].s = 0;
        e[i].t = (i % 4 == 3) ? 0 : 9;
      }
      b = b || e[0].s;
      c = e[0].t;
    }
  return 0;
}
