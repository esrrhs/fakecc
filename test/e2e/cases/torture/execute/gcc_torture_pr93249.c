// expect: 0
package main;

/* PR tree-optimization/93249 */

extern void abort (void);
extern char *strncpy (char *, const char *, unsigned long);

char a[2], b[4], c[6];

void
foo (void)
{
  char d[2] = { 0x00, 0x11 };
  strncpy (&b[2], d, 2);
  strncpy (&b[1], a, 2);
  if (b[0] || b[1] || b[2] || b[3])
    abort ();
}

void
bar (void)
{
  strncpy (&b[2], "\0\x11", 2);
  strncpy (&b[1], a, 2);
  if (b[0] || b[1] || b[2] || b[3])
    abort ();
}

void
baz (void)
{
  strncpy (&c[2], "\x11\x11\0\x11", 4);
  strncpy (&c[1], a, 2);
  if (c[0] || c[1] || c[2] || c[3] != 0x11 || c[4] || c[5])
    abort ();
}

int
main (void)
{
  foo ();
  bar ();
  baz ();
  return 0;
}
