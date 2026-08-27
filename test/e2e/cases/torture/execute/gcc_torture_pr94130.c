// expect: 0
package main;

/* PR tree-optimization/94130 */

extern void abort (void);
extern void *memset (void *, int, unsigned long);

int
main (void)
{
  int a[8];
  char *b = memset (a, 0, sizeof (a));
  a[0] = 1;
  a[1] = 2;
  a[2] = 3;
  if (b != (char *) a)
    abort ();
  return 0;
}
