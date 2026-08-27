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
typedef long unsigned int size_t;
extern void abort (void);

__attribute__ ((__noinline__))
size_t
strnlen (const char *s, size_t n)
{
  size_t i;
  i = 0;
  while (s[i] != 0 && n--)
    i++;
  return i;
}
typedef long unsigned int size_t;
extern void abort (void);
extern size_t strnlen (const char *, size_t);
__attribute__ ((noipa)) void test_strnlen_str_cst (void)
{
  ((strnlen ("", 0) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 21, "strnlen (\"\", 0) == 0"), abort ()));
  ((strnlen ("", 1) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 22, "strnlen (\"\", 1) == 0"), abort ()));
  ((strnlen ("", 9) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 23, "strnlen (\"\", 9) == 0"), abort ()));
  ((strnlen ("", 0x7fffffffffffffffL) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 24, "strnlen (\"\", PTRDIFF_MAX) == 0"), abort ()));
  ((strnlen ("", 0xffffffffffffffffUL) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 25, "strnlen (\"\", SIZE_MAX) == 0"), abort ()));
  ((strnlen ("", -1) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 26, "strnlen (\"\", -1) == 0"), abort ()));
  ((strnlen ("1", 0) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 28, "strnlen (\"1\", 0) == 0"), abort ()));
  ((strnlen ("1", 1) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 29, "strnlen (\"1\", 1) == 1"), abort ()));
  ((strnlen ("1", 9) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 30, "strnlen (\"1\", 9) == 1"), abort ()));
  ((strnlen ("1", 0x7fffffffffffffffL) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 31, "strnlen (\"1\", PTRDIFF_MAX) == 1"), abort ()));
  ((strnlen ("1", 0xffffffffffffffffUL) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 32, "strnlen (\"1\", SIZE_MAX) == 1"), abort ()));
  ((strnlen ("1", -2) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 33, "strnlen (\"1\", -2) == 1"), abort ()));
  ((strnlen ("123", 0) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 35, "strnlen (\"123\", 0) == 0"), abort ()));
  ((strnlen ("123", 1) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 36, "strnlen (\"123\", 1) == 1"), abort ()));
  ((strnlen ("123", 2) == 2) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 37, "strnlen (\"123\", 2) == 2"), abort ()));
  ((strnlen ("123", 3) == 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 38, "strnlen (\"123\", 3) == 3"), abort ()));
  ((strnlen ("123", 9) == 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 39, "strnlen (\"123\", 9) == 3"), abort ()));
  ((strnlen ("123", 0x7fffffffffffffffL) == 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 40, "strnlen (\"123\", PTRDIFF_MAX) == 3"), abort ()));
  ((strnlen ("123", 0xffffffffffffffffUL) == 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 41, "strnlen (\"123\", SIZE_MAX) == 3"), abort ()));
  ((strnlen ("123", -2) == 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 42, "strnlen (\"123\", -2) == 3"), abort ()));
}
__attribute__ ((noipa)) void test_strnlen_str_range (size_t x)
{
  size_t r_0_3 = x & 3;
  size_t r_1_3 = r_0_3 | 1;
  size_t r_2_3 = r_0_3 | 2;
  ((strnlen ("", r_0_3) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 51, "strnlen (\"\", r_0_3) == 0"), abort ()));
  ((strnlen ("1", r_0_3) <= 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 52, "strnlen (\"1\", r_0_3) <= 1"), abort ()));
  ((strnlen ("12", r_0_3) <= 2) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 53, "strnlen (\"12\", r_0_3) <= 2"), abort ()));
  ((strnlen ("123", r_0_3) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 54, "strnlen (\"123\", r_0_3) <= 3"), abort ()));
  ((strnlen ("1234", r_0_3) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 55, "strnlen (\"1234\", r_0_3) <= 3"), abort ()));
  ((strnlen ("", r_1_3) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 57, "strnlen (\"\", r_1_3) == 0"), abort ()));
  ((strnlen ("1", r_1_3) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 58, "strnlen (\"1\", r_1_3) == 1"), abort ()));
  ((strnlen ("12", r_1_3) <= 2) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 59, "strnlen (\"12\", r_1_3) <= 2"), abort ()));
  ((strnlen ("123", r_1_3) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 60, "strnlen (\"123\", r_1_3) <= 3"), abort ()));
  ((strnlen ("1234", r_1_3) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 61, "strnlen (\"1234\", r_1_3) <= 3"), abort ()));
  ((strnlen ("", r_2_3) == 0) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 63, "strnlen (\"\", r_2_3) == 0"), abort ()));
  ((strnlen ("1", r_2_3) == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 64, "strnlen (\"1\", r_2_3) == 1"), abort ()));
  ((strnlen ("12", r_2_3) == 2) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 65, "strnlen (\"12\", r_2_3) == 2"), abort ()));
  ((strnlen ("123", r_2_3) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 66, "strnlen (\"123\", r_2_3) <= 3"), abort ()));
  ((strnlen ("1234", r_2_3) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 67, "strnlen (\"1234\", r_2_3) <= 3"), abort ()));
}
__attribute__ ((noipa)) void test_strnlen_str_range_side_effect (size_t x)
{
  size_t r_0_3 = x & 3;
  size_t r_1_3 = r_0_3 | 1;
  size_t r_2_3 = r_0_3 | 2;
  size_t n = r_2_3;
  int i = 0;
  ((strnlen ("1234" + i++, n) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 79, "strnlen (\"1234\" + i++, n) <= 3"), abort ()));
  ((i == 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 80, "i == 1"), abort ()));
  ((strnlen ("1234", n++) <= 3) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 82, "strnlen (\"1234\", n++) <= 3"), abort ()));
  ((n == r_2_3 + 1) ? (void)0 : (__builtin_printf ("assertion on line %i failed: %s\n", 83, "n == r_2_3 + 1"), abort ()));
}
void
main_test (void)
{
  test_strnlen_str_cst ();
  test_strnlen_str_range ((size_t)"");
  test_strnlen_str_range_side_effect ((size_t)"");
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
