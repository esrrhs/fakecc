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
extern void abort(void);

__attribute__ ((__noinline__))
void *
memchr (const void *s, int c, long unsigned int n)
{
  const unsigned char uc = c;
  const unsigned char *sp;
  sp = s;
  for (; n != 0; ++sp, --n)
    if (*sp == uc)
      return (void *) sp;
  return 0;
}
extern void abort (void);
typedef long unsigned int size_t;
extern void *memchr (const void *, int, size_t);
void
main_test (void)
{
  const char* const foo1 = "hello world";
  if (memchr (foo1, 'x', 11))
    abort ();
  if (memchr (foo1, 'o', 11) != foo1 + 4)
    abort ();
  if (memchr (foo1, 'w', 2))
    abort ();
  if (memchr (foo1 + 5, 'o', 6) != foo1 + 7)
    abort ();
  if (memchr (foo1, 'd', 11) != foo1 + 10)
    abort ();
  if (memchr (foo1, 'd', 10))
    abort ();
  if (memchr (foo1, '\0', 11))
    abort ();
  if (memchr (foo1, '\0', 12) != foo1 + 11)
    abort ();
  if (__builtin_memchr (foo1, 'r', 11) != foo1 + 8)
    abort ();
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
