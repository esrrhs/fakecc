// expect: 0
package main;

/* PR rtl-optimization/97386 */

extern void abort (void);

unsigned char
foo (unsigned int c)
{
  return __builtin_bswap16 ((unsigned long long) (0xccccLLU << c | 0xccccLLU >> ((-c) & 63)));
}

int
main ()
{
  unsigned char x = foo (0);
  if (x != 0xcc)
    abort ();
  return 0;
}
