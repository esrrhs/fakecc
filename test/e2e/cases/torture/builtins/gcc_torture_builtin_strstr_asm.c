// expect: 0
package main;

/* Port of gcc.c-torture/execute/builtins/strstr-asm.c
 * (+ strstr-asm-lib.c + lib/main.c). ASMNAME expanded; strstr is redirected
 * with a GNU __asm alias. Original abort checks kept. */

extern void abort (void);
typedef unsigned long size_t;

extern char *strstr (const char *, const char *)
  __asm ("my_strstr");

extern size_t strlen (const char *);
extern char *strchr (const char *, int);
extern int strcmp (const char *, const char *);
extern int strncmp (const char *, const char *, size_t);

const char *p = "rld", *q = "hello world";
int inside_main = 0;

__attribute__ ((used))
char *
my_strstr (const char *s1, const char *s2)
{
  const size_t len = strlen (s2);
  if (len == 0)
    return (char *) s1;
  for (s1 = strchr (s1, *s2); s1; s1 = strchr (s1 + 1, *s2))
    if (strncmp (s1, s2, len) == 0)
      return (char *) s1;
  return (char *) 0;
}

void
main_test (void)
{
  const char *const foo = "hello world";

  if (strstr (foo, "") != foo)
    abort ();
  if (strstr (foo + 4, "") != foo + 4)
    abort ();
  if (strstr (foo, "h") != foo)
    abort ();
  if (strstr (foo, "w") != foo + 6)
    abort ();
  if (strstr (foo + 6, "o") != foo + 7)
    abort ();
  if (strstr (foo + 1, "world") != foo + 6)
    abort ();
  if (strstr (foo + 2, p) != foo + 8)
    abort ();
  if (strstr (q, "") != q)
    abort ();
  if (strstr (q + 1, "o") != q + 4)
    abort ();
  if (__builtin_strstr (foo + 1, "world") != foo + 6)
    abort ();
}

int main (void)
{
  inside_main = 1;
  main_test ();
  inside_main = 0;
  return 0;
}
