// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/complex-3.c
package main;

struct complex
{
  float r;
  float i;
};

struct complex cmplx (float, float);

struct complex
f (float a, float b)
{
  struct complex c;
  c.r = a;
  c.i = b;
  return c;
}

int
main (void)
{
  struct complex z = f (1.0, 0.0);

  if (z.r != 1.0 || z.i != 0.0)
    return 1;
  return 0;
}