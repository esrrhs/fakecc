// expect: 0
package main;

/* PR tree-optimization/56982 */

extern void abort (void);
extern void exit (int);

static void *env[5];

void baz (void)
{
}

static inline int g (int x)
{
  if (x)
    {
      baz ();
      return 0;
    }
  else
    {
      baz ();
      return 1;
    }
}

int f (int *e)
{
  if (*e)
    return 1;

  int x = __builtin_setjmp (env);
  int n = g (x);
  if (n == 0)
    exit (0);
  if (x)
    abort ();
  __builtin_longjmp (env, 1);
}

int main (int argc, char **argv)
{
  int v = 0;
  return f (&v);
}
