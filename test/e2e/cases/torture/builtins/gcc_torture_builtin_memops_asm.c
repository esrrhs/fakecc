// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/memops-asm.c
 * (+ memops-asm-lib.c + lib/main.c). ASMNAME macros expanded; memcpy/memset/
 * bcopy/bzero/memmove are redirected with GNU __asm aliases. */

extern void abort (void);
typedef unsigned long size_t;

extern int memcmp (const void *, const void *, size_t);

extern void *memcpy (void *, const void *, size_t)
  __asm ("my_memcpy");
extern void bcopy (const void *, void *, size_t)
  __asm ("my_bcopy");
extern void *memmove (void *, const void *, size_t)
  __asm ("my_memmove");
extern void *memset (void *, int, size_t)
  __asm ("my_memset");
extern void bzero (void *, size_t)
  __asm ("my_bzero");

__attribute__ ((used))
void *
my_memcpy (void *d, const void *s, size_t n)
{
  char *dst = (char *) d;
  const char *src = (const char *) s;
  while (n--)
    *dst++ = *src++;
  return (char *) d;
}

__attribute__ ((used))
void
my_bcopy (const void *s, void *d, size_t n)
{
  char *dst = (char *) d;
  const char *src = (const char *) s;
  if (src >= dst)
    while (n--)
      *dst++ = *src++;
  else
    {
      dst += n;
      src += n;
      while (n--)
        *--dst = *--src;
    }
}

__attribute__ ((used))
void *
my_memmove (void *d, const void *s, size_t n)
{
  char *dst = (char *) d;
  const char *src = (const char *) s;
  if (src >= dst)
    while (n--)
      *dst++ = *src++;
  else
    {
      dst += n;
      src += n;
      while (n--)
        *--dst = *--src;
    }
  return d;
}

__attribute__ ((used))
void *
my_memset (void *d, int c, size_t n)
{
  char *dst = (char *) d;
  while (n--)
    *dst++ = c;
  return (char *) d;
}

__attribute__ ((used))
void
my_bzero (void *d, size_t n)
{
  char *dst = (char *) d;
  while (n--)
    *dst++ = '\0';
}

struct A { char c[32]; } a = { "foobar" };
char x[64] = "foobar", y[64];
int i = 39, j = 6, k = 4;

int inside_main = 0;

void
main_test (void)
{
  struct A b = a;
  struct A c = { { 'x' } };

  inside_main = 1;

  if (memcmp (b.c, x, 32) || c.c[0] != 'x' || memcmp (c.c + 1, x + 32, 31))
    abort ();
  if (__builtin_memcpy (y, x, i) != y || memcmp (x, y, 64))
    abort ();
  if (memcpy (y + 6, x, j) != y + 6
      || memcmp (x, y, 6) || memcmp (x, y + 6, 58))
    abort ();
  if (__builtin_memset (y + 2, 'X', k) != y + 2
      || memcmp (y, "foXXXXfoobar", 13))
    abort ();
  bcopy (y + 1, y + 2, 6);
  if (memcmp (y, "fooXXXXfobar", 13))
    abort ();
  __builtin_bzero (y + 4, 2);
  if (memcmp (y, "fooX\0\0Xfobar", 13))
    abort ();
}

int main (void)
{
  main_test ();
  inside_main = 0;
  return 0;
}
