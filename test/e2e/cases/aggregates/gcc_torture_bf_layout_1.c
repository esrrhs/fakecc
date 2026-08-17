// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/bf-layout-1.c
package main;

struct { long f8:8; long f24:24; } a;
struct { long f32:32; } b;

int
main (void)
{
  if (sizeof (a) != sizeof (b))
    return 1;
  return 0;
}