// expect: 0
package main;

/* Test to verify that a strlen() call with a pointer to a dynamic type
   doesn't make assumptions based on the static type of the original
   pointer.  See g++.dg/init/strlen.C for the corresponding C++ test.  */

extern void abort (void);
extern void *malloc (unsigned long);
extern void *memcpy (void *, const void *, unsigned long);
extern char *strcpy (char *, const char *);
extern unsigned long strlen (const char *);

struct A { int i; char a[1]; void (*p)(); };
struct B { char a[sizeof (struct A) - __builtin_offsetof (struct A, a)]; };

void
init (char *d, const char *s)
{
  strcpy (d, s);
}

struct B b;

void
test_dynamic_type (struct A *p)
{
  char *q = (char *)memcpy (p->a, &b, sizeof b);

  init (q, "foobar");

  if (6 != strlen (q))
    abort ();
}

int main (void)
{
  struct A *p = (struct A *)malloc (sizeof *p);
  test_dynamic_type (p);
  return 0;
}
