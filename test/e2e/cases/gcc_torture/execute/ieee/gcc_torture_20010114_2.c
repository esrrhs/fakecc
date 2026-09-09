// expect: 0
package main;

extern void exit (int);
extern void abort (void);

float
rintf (float x)
{
  static const float TWO23 = 8388608.0f;

  if (__builtin_fabsf (x) < TWO23)
    {
      if (x > 0.0f)
        {
          x += TWO23;
          x -= TWO23;
        }
      else if (x < 0.0f)
        {
          x = TWO23 - x;
          x = -(x - TWO23);
        }
    }

  return x;
}

int main (void)
{
  if (rintf (-1.5f) != -2.0f)
    abort ();
  exit (0);
}
