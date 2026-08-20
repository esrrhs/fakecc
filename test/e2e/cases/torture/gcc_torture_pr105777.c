// expect: 0
package main;
int foo(int x) { int r; return __builtin_mul_overflow(x, 35, &r); }
long bar(long x) { long r; return __builtin_mul_overflow(x, 35, &r); }
int baz(int x) { int r; return __builtin_mul_overflow(x, 42, &r); }
long qux(long x) { long r; return __builtin_mul_overflow(x, 42, &r); }
int corge(int x) { int r; return __builtin_mul_overflow(x, -39, &r); }
long garply(long x) { long r; return __builtin_mul_overflow(x, -39, &r); }
int grault(int x) { int r; return __builtin_mul_overflow(x, -46, &r); }
long waldo(long x) { long r; return __builtin_mul_overflow(x, -46, &r); }
int
main ()
{
  if (foo (0) != 0
      || foo (0x7fffffff / 35) != 0
      || foo (0x7fffffff / 35 + 1) != 1
      || foo (0x7fffffff) != 1
      || foo ((-0x7fffffff - 1) / 35) != 0
      || foo ((-0x7fffffff - 1) / 35 - 1) != 1
      || foo (-0x7fffffff - 1) != 1)
    __builtin_abort ();
  if (bar (0) != 0
      || bar (0x7fffffffffffffffL / 35) != 0
      || bar (0x7fffffffffffffffL / 35 + 1) != 1
      || bar (0x7fffffffffffffffL) != 1
      || bar ((-0x7fffffffffffffffL - 1) / 35) != 0
      || bar ((-0x7fffffffffffffffL - 1) / 35 - 1) != 1
      || bar (-0x7fffffffffffffffL - 1) != 1)
    __builtin_abort ();
  if (baz (0) != 0
      || baz (0x7fffffff / 42) != 0
      || baz (0x7fffffff / 42 + 1) != 1
      || baz (0x7fffffff) != 1
      || baz ((-0x7fffffff - 1) / 42) != 0
      || baz ((-0x7fffffff - 1) / 42 - 1) != 1
      || baz (-0x7fffffff - 1) != 1)
    __builtin_abort ();
  if (qux (0) != 0
      || qux (0x7fffffffffffffffL / 42) != 0
      || qux (0x7fffffffffffffffL / 42 + 1) != 1
      || qux (0x7fffffffffffffffL) != 1
      || qux ((-0x7fffffffffffffffL - 1) / 42) != 0
      || qux ((-0x7fffffffffffffffL - 1) / 42 - 1) != 1
      || qux (-0x7fffffffffffffffL - 1) != 1)
    __builtin_abort ();
  if (corge (0) != 0
      || corge (0x7fffffff / -39) != 0
      || corge (0x7fffffff / -39 - 1) != 1
      || corge (0x7fffffff) != 1
      || corge ((-0x7fffffff - 1) / -39) != 0
      || corge ((-0x7fffffff - 1) / -39 + 1) != 1
      || corge (-0x7fffffff - 1) != 1)
    __builtin_abort ();
  if (garply (0) != 0
      || garply (0x7fffffffffffffffL / -39) != 0
      || garply (0x7fffffffffffffffL / -39 - 1) != 1
      || garply (0x7fffffffffffffffL) != 1
      || garply ((-0x7fffffffffffffffL - 1) / -39) != 0
      || garply ((-0x7fffffffffffffffL - 1) / -39 + 1) != 1
      || garply (-0x7fffffffffffffffL - 1) != 1)
    __builtin_abort ();
  if (grault (0) != 0
      || grault (0x7fffffff / -46) != 0
      || grault (0x7fffffff / -46 - 1) != 1
      || grault (0x7fffffff) != 1
      || grault ((-0x7fffffff - 1) / -46) != 0
      || grault ((-0x7fffffff - 1) / -46 + 1) != 1
      || grault (-0x7fffffff - 1) != 1)
    __builtin_abort ();
  if (waldo (0) != 0
      || waldo (0x7fffffffffffffffL / -46) != 0
      || waldo (0x7fffffffffffffffL / -46 - 1) != 1
      || waldo (0x7fffffffffffffffL) != 1
      || waldo ((-0x7fffffffffffffffL - 1) / -46) != 0
      || waldo ((-0x7fffffffffffffffL - 1) / -46 + 1) != 1
      || waldo (-0x7fffffffffffffffL - 1) != 1)
    __builtin_abort ();
  return 0;
}
