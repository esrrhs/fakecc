// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20120817-1.c
package main;

typedef unsigned long long u64;
unsigned long foo = 0;
u64 f() ;

u64 f() {
  return ((u64)40) + ((u64) 24) * (int)(foo - 1);
}

int main ()
{
  if (f () != 16)
    return 1;
  return 0;
}