// expect: 0
package main;

extern void abort(void);

typedef unsigned long uintptr_t;

__attribute__ ((noclone, noinline)) char* get_max_2 (char *p)
{
  return p + 1;
}

__attribute__ ((noclone, noinline)) char* get_max_3 (char *p, char *q)
{
  return p < q ? q + 1 : p + 1;
}

__attribute__ ((noclone, noinline)) char* get_min_2 (char *p)
{
  return p - 1;
}

__attribute__ ((noclone, noinline)) char* get_min_3 (char *p, char *q)
{
  return p < q ? p - 1 : q - 1;
}

__attribute__ ((noclone, noinline)) void* test_max_2 (void)
{
  char c;

  char *p = get_max_2 (&c);

  void *q = p > &c ? p : &c;  /* MAX_EXPR */
  return q;
}

__attribute__ ((noclone, noinline)) void* test_max_3 (void)
{
  char c;
  char d;

  char *p = get_max_3 (&c, &d);

  void *q = p < &c ? &c < &d ? &d : &c : p;
  return q;
}

__attribute__ ((noclone, noinline)) void* test_min_2 (void)
{
  char c;

  char *p = get_min_2 (&c);

  void *q = p < &c ? p : &c;  /* MIN_EXPR" */
  return q;
}

__attribute__ ((noclone, noinline)) void* test_min_3 (void)
{
  char c;
  char d;

  char *p = get_min_3 (&c, &d);

  void *q = p > &c ? &c > &d ? &d : &c : p;
  return q;
}

__attribute__ ((noclone, noinline)) void* test_min_3_phi (int i)
{
  char a, b;

  char *p0 = &a;
  char *p1 = &b;
  char *p2 = get_min_3 (&a, &b);
  char *p3 = get_min_3 (&a, &b);

  char *p4 = p2 < p0 ? p2 : p0;
  char *p5 = p3 < p1 ? p3 : p1;

  if (i == 1)
    return p4;
  else
    return p5;
}

int main ()
{
  if (0 == test_max_2 ()) abort();
  if (0 == test_max_3 ()) abort();

  if (0 == test_min_2 ()) abort();
  if (0 == test_min_3 ()) abort();

  if (0 == test_min_3_phi (0)) abort();
  return 0;
}
