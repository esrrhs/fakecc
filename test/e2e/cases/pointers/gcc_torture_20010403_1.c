// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010403-1.c
package main;

void b (int *);
int c (int, int);
void d (int);

int e;

void a (int x, int y)
{
  int f = x ? e : 0;
  int z = y;

  b (&y);
  c (z, y);
  d (f);
}

void b (int *y)
{
  (*y)++;
}

int c (int x, int y)
{
  if (x == y)
    return 1;

    return 0;}

void d (int x)
{
}

int main (void)
{
  a (0, 0);
  return 0;
}