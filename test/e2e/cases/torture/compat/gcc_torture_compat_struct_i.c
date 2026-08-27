// expect: 0
package main;

extern void abort(void);
extern void exit(int);

typedef struct {int a;} T;

int h (T *x)
{
  if (x->a != 47114711)
    abort ();
  return 0;
}

T
g (T x)
{
  if (x.a != 13)
    abort ();
  x.a = 47114711;
  h (&x);
  return x;
}

int f (void)
{
  T x;
  x.a = 13;
  g (x);
  if (x.a != 13)
    abort ();
  x = g (x);
  if (x.a != 47114711)
    abort ();
  return 0;
}

int main (void)
{
  f ();
  exit (0);
}
