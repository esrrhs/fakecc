// expect: 0
package main;

extern void abort (void);

/* PR target/92904 */


struct S { long long a, b; };
struct __attribute__((aligned (16))) T { long long a, b; };
struct U { double a, b, c, d; };
struct __attribute__((aligned (32))) V { double a, b, c, d; };
struct W { double a; long long b; };
struct __attribute__((aligned (16))) X { double a; long long b; };
struct S c;
struct T d;
struct U e;
struct V f;
struct W g;
struct X h;


 struct S
f2 (int x, ...)
{
  struct S r;
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, int);
  r = __builtin_va_arg (ap, struct S);
  __builtin_va_end (ap);
  return r;
}

 struct T
f3 (int x, ...)
{
  struct T r;
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, int);
  r = __builtin_va_arg (ap, struct T);
  __builtin_va_end (ap);
  return r;
}


 void
f5 (int x, ...)
{
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, int);
  c = __builtin_va_arg (ap, struct S);
  __builtin_va_end (ap);
}

 void
f6 (int x, ...)
{
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, int);
  d = __builtin_va_arg (ap, struct T);
  __builtin_va_end (ap);
}

 struct U
f7 (int x, ...)
{
  struct U r;
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, double);
  r = __builtin_va_arg (ap, struct U);
  __builtin_va_end (ap);
  return r;
}

 struct V
f8 (int x, ...)
{
  struct V r;
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, double);
  r = __builtin_va_arg (ap, struct V);
  __builtin_va_end (ap);
  return r;
}

 void
f9 (int x, ...)
{
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, double);
  e = __builtin_va_arg (ap, struct U);
  __builtin_va_end (ap);
}

 void
f10 (int x, ...)
{
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    __builtin_va_arg (ap, double);
  f = __builtin_va_arg (ap, struct V);
  __builtin_va_end (ap);
}

 struct W
f11 (int x, ...)
{
  struct W r;
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    {
      __builtin_va_arg (ap, int);
      __builtin_va_arg (ap, double);
    }
  r = __builtin_va_arg (ap, struct W);
  __builtin_va_end (ap);
  return r;
}

 struct X
f12 (int x, ...)
{
  struct X r;
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    {
      __builtin_va_arg (ap, int);
      __builtin_va_arg (ap, double);
    }
  r = __builtin_va_arg (ap, struct X);
  __builtin_va_end (ap);
  return r;
}

 void
f13 (int x, ...)
{
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    {
      __builtin_va_arg (ap, int);
      __builtin_va_arg (ap, double);
    }
  g = __builtin_va_arg (ap, struct W);
  __builtin_va_end (ap);
}

 void
f14 (int x, ...)
{
  __builtin_va_list ap;
  __builtin_va_start (ap, x);
  while (x--)
    {
      __builtin_va_arg (ap, int);
      __builtin_va_arg (ap, double);
    }
  h = __builtin_va_arg (ap, struct X);
  __builtin_va_end (ap);
}

int
main ()
{
  union Y {
    struct S c;
    struct T d;
    struct U e;
    struct V f;
    struct W g;
    struct X h;
  } u, v;
  u.c.a = 0x5555555555555555ULL;
  u.c.b = 0xaaaaaaaaaaaaaaaaULL;
  v.c = f2 (0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (1, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (2, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (3, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (4, 0, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (5, 0, 0, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (6, 0, 0, 0, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (7, 0, 0, 0, 0, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (8, 0, 0, 0, 0, 0, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.c = f2 (9, 0, 0, 0, 0, 0, 0, 0, 0, 0, u.c); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (1, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (2, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (3, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (4, 0, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (5, 0, 0, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (6, 0, 0, 0, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (7, 0, 0, 0, 0, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (8, 0, 0, 0, 0, 0, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  v.d = f3 (9, 0, 0, 0, 0, 0, 0, 0, 0, 0, u.d); if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (1, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (2, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (3, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (4, 0, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (5, 0, 0, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (6, 0, 0, 0, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (7, 0, 0, 0, 0, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (8, 0, 0, 0, 0, 0, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f5 (9, 0, 0, 0, 0, 0, 0, 0, 0, 0, u.c); v.c = c; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (1, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (2, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (3, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (4, 0, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (5, 0, 0, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (6, 0, 0, 0, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (7, 0, 0, 0, 0, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (8, 0, 0, 0, 0, 0, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  f6 (9, 0, 0, 0, 0, 0, 0, 0, 0, 0, u.d); v.d = d; if (u.c.a != v.c.a || u.c.b != v.c.b) abort (); u.c.a++; u.c.b--;
  u.e.a = 1.25;
  u.e.b = 2.75;
  u.e.c = -3.5;
  u.e.d = -2.0;
  v.e = f7 (0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (1, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (2, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (3, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (4, 0.0, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (5, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.e = f7 (9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (1, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (2, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (3, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (4, 0.0, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (5, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  v.f = f8 (9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (1, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (2, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (3, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (4, 0.0, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (5, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f9 (9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.e); v.e = e; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (1, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (2, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (3, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (4, 0.0, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (5, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (6, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (7, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  f10 (9, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, u.f); v.f = f; if (u.e.a != v.e.a || u.e.b != v.e.b || u.e.c != v.e.c || u.e.d != v.e.d) abort (); u.e.a++; u.e.b--; u.e.c++; u.e.d--;
  u.g.a = 9.5;
  u.g.b = 0x5555555555555555ULL;
  v.g = f11 (0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (1, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (2, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (3, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (4, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (5, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (6, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (7, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (8, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.g = f11 (9, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (1, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (2, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (3, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (4, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (5, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (6, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (7, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (8, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  v.h = f12 (9, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (1, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (2, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (3, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (4, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (5, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (6, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (7, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (8, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f13 (9, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.g); v.g = g; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (1, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (2, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (3, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (4, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (5, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (6, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (7, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (8, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  f14 (9, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, 0, 0.0, u.h); v.h = h; if (u.e.a != v.e.a || u.e.b != v.e.b) abort (); u.e.a++; u.e.b--;
  return 0;
}
