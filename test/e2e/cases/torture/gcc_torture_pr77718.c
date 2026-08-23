/* PR middle-end/77718 */

// expect: 0
package main;

char a[64];

__attribute__((noinline)) int
foo (void)
{
  return __builtin_memcmp ("bbbbbb", a, 6);
}

__attribute__((noinline)) int
bar (void)
{
  return __builtin_memcmp (a, "bbbbbb", 6);
}

int
main (void)
{
  __builtin_memset (a, 'a', 64);
  if (((foo () < 0) ^ ('a' > 'b'))
      || ((bar () < 0) ^ ('a' < 'b')))
    __builtin_abort ();
  return 0;
}
