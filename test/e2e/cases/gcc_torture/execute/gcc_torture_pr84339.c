// expect: 0
package main;

/* PR tree-optimization/84339 */

extern void abort (void);
extern void *malloc (unsigned long);
extern void free (void *);
extern unsigned long strlen (const char *);
extern char *strcpy (char *, const char *);

struct S { int a; char b[1]; };

int
foo (struct S *p)
{
  return strlen (&p->b[0]);
}

int
bar (struct S *p)
{
  return strlen (p->b);
}

int
main (void)
{
  struct S *p = malloc (sizeof (struct S) + 16);
  if (p)
    {
      p->a = 1;
      strcpy (p->b, "abcdefg");
      if (foo (p) != 7 || bar (p) != 7)
	abort ();
      free (p);
    }
  return 0;
}
