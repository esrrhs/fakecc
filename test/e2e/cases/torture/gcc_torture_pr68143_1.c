// expect: 0
package main;

struct stuff
{
    int a;
    int b;
    int c;
    int d;
    int e;
    char *f;
    int g;
};

void __attribute__ ((noinline))
bar (struct stuff *x)
{
  if (x->g != 2)
    __builtin_abort ();
}

int
main (void)
{
  struct stuff x;
  char *null = 0;
  x.a = 0; x.b = 0; x.c = 0; x.d = 0; x.e = 0; x.f = null; x.g = 0;
  x.a = 100;
  x.d = 100;
  x.g = 2;
  bar (&x);
  return 0;
}
