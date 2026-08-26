// expect: 0
package main;

/* PR tree-optimization/72824 */

extern void abort(void);

static inline void
foo (float *x, float value)
{
  int i;
  for (i = 0; i < 32; ++i)
    x[i] = value;
}

int
main ()
{
  float x[32];
  foo (x, -0.f);
  if (__builtin_copysignf (1.0f, x[3]) != -1.0f)
    abort ();
  return 0;
}
