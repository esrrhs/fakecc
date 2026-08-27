// expect: 0
package main;

/* PR tree-optimization/88693 */

extern void abort (void);
extern unsigned long strlen (const char *);
extern void *memcpy (void *, const void *, unsigned long);
extern void *memset (void *, int, unsigned long);

void
foo (char *p)
{
  if (strlen (p) != 9)
    abort ();
}

void
quux (char *p)
{
  int i;
  for (i = 0; i < 100; i++)
    if (p[i] != 'x')
      abort ();
}

void
qux (void)
{
  char b[100];
  memset (b, 'x', sizeof (b));
  quux (b);
}

void
bar (void)
{
  static unsigned char u[9] = "abcdefghi";
  char b[100];
  memcpy (b, u, sizeof (u));
  b[sizeof (u)] = 0;
  foo (b);
}

void
baz (void)
{
  static unsigned char u[] = { 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r' };
  char b[100];
  memcpy (b, u, sizeof (u));
  b[sizeof (u)] = 0;
  foo (b);
}

int
main (void)
{
  qux ();
  bar ();
  baz ();
  return 0;
}
