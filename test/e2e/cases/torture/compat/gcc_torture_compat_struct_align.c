// expect: 0
package main;

extern void abort(void);
extern void exit(int);

typedef union
{
  struct {int a; int b;} s;
  double d;
} T;

int h (T *x)
{
  if (x->s.a != 0 || x->s.b != 1)
    abort ();
  return 0;
}

T
g (T x)
{
  if (x.s.a != 13 || x.s.b != 47)
    abort ();
  x.s.a = 0;
  x.s.b = 1;
  h (&x);
  return x;
}

int f (void)
{
  T x;
  x.s.a = 13;
  x.s.b = 47;
  g (x);
  if (x.s.a != 13 || x.s.b != 47)
    abort ();
  x = g (x);
  if (x.s.a != 0 || x.s.b != 1)
    abort ();
  return 0;
}

int main (void)
{
  f ();
  exit (0);
}
