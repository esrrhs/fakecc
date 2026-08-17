// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/951204-1.c
package main;

void
f (char *x)
{
  *x = 'x';
}

int
main (void)
{
  int i;
  char x = '\0';

  for (i = 0; i < 100; ++i)
    {
      f (&x);
      if (*(const char *) &x != 'x')
	return 1;
    }
  return 0;
}