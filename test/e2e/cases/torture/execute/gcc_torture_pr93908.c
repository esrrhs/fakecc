/* PR rtl-optimization/93908 */

// expect: 0
package main;

struct T
{
  int b;
  int c;
  unsigned short d;
  unsigned e:1, f:1, g:1, h:2, i:1, j:1;
  signed int k:2;
};

struct S
{
  struct T s;
  char c[64];
} buf[2];

void *
baz (void)
{
  static int cnt;
  return (void *) &buf[cnt++];
}

static inline struct T *
bar (const char *a)
{
  struct T *s;
  s = baz ();
  s->b = 1;
  s->k = -1;
  return s;
}

void
foo (const char *x, struct T **y)
{
  struct T *l = bar (x);
  struct T *m = bar (x);
  y[0] = l;
  y[1] = m;
}

int
main (void)
{
  struct T *r[2];
  foo ("foo", r);
  if (r[0]->e || r[0]->f || r[0]->g || r[0]->h || r[0]->i || r[0]->j || r[0]->k != -1)
    __builtin_abort ();
  if (r[1]->e || r[1]->f || r[1]->g || r[1]->h || r[1]->i || r[1]->j || r[1]->k != -1)
    __builtin_abort ();
  return 0;
}
