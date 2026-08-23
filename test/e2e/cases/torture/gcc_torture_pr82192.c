/* PR rtl-optimization/82192 */

// expect: 0
package main;

unsigned long long int a = 0x95dd3d896f7422e2ULL;
struct S { unsigned int m : 13; } b;

__attribute__((noinline)) void
foo (void)
{
  b.m = ((unsigned) a) >> (0x644eee9667723bf7LL
			   | a & ~0xdee27af8U) - 0x644eee9667763bd8LL;
}

int
main (void)
{
  foo ();
  if (b.m != 0)
    __builtin_abort ();
  return 0;
}
