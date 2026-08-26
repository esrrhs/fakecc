// expect: 0
package main;

/* PR tree-optimization/79737 */

extern void abort (void);

typedef int int32_t;

struct __attribute__ ((packed)) S
{
  int32_t b:18;
  int32_t c:1;
  int32_t d:24;
  int32_t e:15;
  int32_t f:14;
} i, j;

void
foo (void)
{
  i.e = 0;
  i.b = 5;
  i.c = 0;
  i.d = -5;
  i.f = 5;
}

void
bar (void)
{
  j.b = 5;
  j.c = 0;
  j.d = -5;
  j.e = 0;
  j.f = 5;
}

int
main (void)
{
  foo ();
  bar ();
  if (i.b != j.b || i.c != j.c || i.d != j.d || i.e != j.e || i.f != j.f)
    abort ();
  return 0;
}
