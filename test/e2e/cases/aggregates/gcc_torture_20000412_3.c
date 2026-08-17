// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000412-3.c
package main;

/* { dg-additional-options "-fpermissive" } */

typedef struct {
  char y;
  char x[32];
} X;

int z (void)
{
  X xxx;
  xxx.x[0] =
  xxx.x[31] = '0';
  xxx.y = 0xf;
  return f (xxx, xxx);
}

int main (void)
{
  int val;

  val = z ();
  if (val != 0x60)
    return 1;
  return 0;
}

int f(X x, X y)
{
  if (x.y != y.y)
    return 'F';

  return x.x[0] + y.x[0];
}