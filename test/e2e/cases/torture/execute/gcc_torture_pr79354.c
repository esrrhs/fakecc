// expect: 0
package main;

/* PR target/79354 */

extern void *memcpy (void *, const void *, unsigned long);

int b, f, g;
float e;
unsigned long d;

void
foo (int *a)
{
  for (g = 0; g < 32; g++)
    if (f)
      {
        e = d;
        memcpy (&b, &e, sizeof (float));
        b = *a;
      }
}

int
main (void)
{
  int h = 5;
  f = 1;
  foo (&h);
  foo (&b);
  return 0;
}
