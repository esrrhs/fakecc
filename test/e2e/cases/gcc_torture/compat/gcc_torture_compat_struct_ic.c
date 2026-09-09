// expect: 0
package main;

extern void abort(void);
extern void exit(int);

typedef struct {int a; char b;} T;

int h (T *x)
{
  if (x->a != 0 || x->b != 1)
    abort ();
  return 0;
}

T
g (T x)
{
  if (x.a != 13 || x.b != 47)
    abort ();
  x.a = 0;
  x.b = 1;
  h (&x);
  return x;
}

int f (void)
{
  T x;
  x.a = 13;
  x.b = 47;
  g (x);
  if (x.a != 13 || x.b != 47)
    abort ();
  x = g (x);
  if (x.a != 0 || x.b != 1)
    abort ();
  return 0;
}

int main (void)
{
  f ();
  exit (0);
}
