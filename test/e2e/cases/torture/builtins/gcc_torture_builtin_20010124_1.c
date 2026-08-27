// expect: 0
package main;

int inside_main = 0;

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
extern int vsnprintf(char *buf, size_t n, const char *fmt, va_list ap);
extern void __fakecc_va_copy(void *dst, void *src);
extern void abort (void);

typedef long unsigned int size_t;
struct S {
  char stuff[1024];
};
union U {
  struct {
    int space;
    struct S s;
  } a;
  struct {
    struct S s;
    int space;
  } b;
};
struct S f(struct S *p)
{
  return *p;
}
void g(union U *p)
{
}
void *memcpy(void *a, const void *b, size_t len)
{
  if (inside_main)
    {
      if (a < b && a+len > b)
        abort ();
      if (b < a && b+len > a)
        abort ();
      return a;
    }
  else
    {
      char *dst = (char *) a;
      const char *src = (const char *) b;
      while (len--)
        *dst++ = *src++;
      return a;
    }
}
extern void abort (void);
struct S f(struct S *);
void g(union U *);
void main_test(void)
{
  union U u;
  u.b.s = f(&u.a.s);
  u.a.s = f(&u.b.s);
  g(&u);
}

int main (void)
{
  inside_main = 1;
  main_test ();
  inside_main = 0;
  return 0;
}

void link_error (void)
{
  abort ();
}
