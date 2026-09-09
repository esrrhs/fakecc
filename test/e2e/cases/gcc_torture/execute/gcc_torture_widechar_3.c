// expect: 0
package main;

extern void abort (void);
extern void exit (int);
extern unsigned long strlen (const char *);

static int f (char *x)
{
   return strlen (x);
}

int foo ()
{
   return f ((char *)&L"abcdef"[0]);
}

int
main ()
{
  if (foo () != 1)
    abort ();
  exit (0);
}
