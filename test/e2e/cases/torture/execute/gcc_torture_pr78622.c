// expect: 0
package main;

/* PR middle-end/78622 */

extern void abort (void);
extern int snprintf (char *, unsigned long, const char *, ...);

int
foo (int x)
{
  if (x < 4096 + 8 || x >= 4096 + 256 + 8)
    return -1;

  char buf[5];
  int n = snprintf (buf, sizeof buf, "%hhd", x + 1);
  return n;
}

int
main (void)
{
  if (sizeof (char) != 1 || sizeof (int) != 4)
    return 0;

  if (foo (4095 + 9) != 1
      || foo (4095 + 32) != 2
      || foo (4095 + 127) != 3
      || foo (4095 + 128) != 4
      || foo (4095 + 240) != 3
      || foo (4095 + 248) != 2
      || foo (4095 + 255) != 2
      || foo (4095 + 256) != 1)
    abort ();

  return 0;
}
