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

__attribute__ ((__noinline__))
long unsigned int
strlen (const char *s)
{
  long unsigned int i;
  i = 0;
  while (s[i] != 0)
    i++;
  return i;
}
extern void abort (void);
extern long unsigned int strlen (const char *);
extern char *strcpy (char *, const char *);
int x = 6;
void
main_test(void)
{
  const char *const foo = "hello world";
  char str[8];
  char *ptr;
  if (strlen (foo) != 11)
    abort ();
  if (strlen (foo + 4) != 7)
    abort ();
  if (strlen (foo + (x++ & 7)) != 5)
    abort ();
  if (x != 7)
    abort ();
  ptr = str;
  ptr[0] = 'n'; ptr[1] = 't'; ptr[2] = 's'; ptr[3] = '\0';
  if (strlen (ptr) == 0)
    abort ();
  ptr[0] = 'n'; ptr[1] = 't'; ptr[2] = 's'; ptr[3] = '\0';
  if (strlen (ptr) < 1)
    abort ();
  ptr[0] = 'n'; ptr[1] = 't'; ptr[2] = 's'; ptr[3] = '\0';
  if (strlen (ptr) <= 0)
    abort ();
  ptr[0] = 'n'; ptr[1] = 't'; ptr[2] = 's'; ptr[3] = '\0';
  if (strlen (ptr+3) != 0)
    abort ();
  ptr[0] = 'n'; ptr[1] = 't'; ptr[2] = 's'; ptr[3] = '\0';
  if (strlen (ptr+3) > 0)
    abort ();
  ptr[0] = 'n'; ptr[1] = 't'; ptr[2] = 's'; ptr[3] = '\0';
  if (strlen (str+3) >= 1)
    abort ();
  if (__builtin_strlen (foo) != 11)
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
