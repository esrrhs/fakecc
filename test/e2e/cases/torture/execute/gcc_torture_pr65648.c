// expect: 0
package main;

/* PR target/65648 */

extern void abort (void);

int a = 0, *b = 0, c = 0;
static int d = 0;
short e = 1;
static long long f = 0;
long long *i = &f;
unsigned char j = 0;

void
foo (int x, int *y)
{
}

void
bar (const char *x, long long y)
{
  if (y != 0)
    abort ();
}

int
main (void)
{
  int k = 0;
  b = &k;
  j = (!a) - (c <= e);
  *i = j;
  foo (a, &k);
  bar ("", f);
  return 0;
}
