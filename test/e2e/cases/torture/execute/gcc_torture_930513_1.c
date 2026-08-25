// expect: 0
package main;

extern int sprintf(char *str, const char *format, ...);
extern void abort(void);
extern void exit(int);

char buf[2];

int f (int (*fp)(char *, const char *, ...))
{
  (*fp)(buf, "%.0f", 5.0);
  return 0;
}

int main (void)
{
  f (&sprintf);
  if (buf[0] != '5' || buf[1] != 0)
    abort ();
  exit (0);
}
