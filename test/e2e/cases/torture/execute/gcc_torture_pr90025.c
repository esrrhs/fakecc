// expect: 0
package main;

/* PR middle-end/90025 */

extern void abort (void);

void
bar (char *p)
{
  int i;
  for (i = 0; i < 6; i++)
    if (p[i] != "foobar"[i])
      abort ();
  for (; i < 32; i++)
    if (p[i] != '\0')
      abort ();
}

void
foo (unsigned int x)
{
  char s[32] = { 'f', 'o', 'o', 'b', 'a', 'r', 0 };
  ((unsigned int *) s)[2] = __builtin_bswap32 (x);
  bar (s);
}

int
main (void)
{
  foo (0);
  return 0;
}
