// expect: 0
package main;

enum { SIZE = 128 };

unsigned short in[SIZE];

__attribute__ ((noinline)) int
test (unsigned short sum, unsigned short *in, int x)
{
  int j;
  for (j = 0; j < SIZE; j += 8)
    sum += in[j] * x;
  return sum;
}

int
main (void)
{
  int i;
  for (i = 0; i < SIZE; i++)
    in[i] = i;
  if (test (0, in, 1) != 960)
    __builtin_abort ();
  return 0;
}
