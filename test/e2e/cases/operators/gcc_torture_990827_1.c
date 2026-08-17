// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990827-1.c
package main;

unsigned test(unsigned one , unsigned  bit)
{
    unsigned val=  bit & 1;
    unsigned zero= one >> 1;

    val++;
    return zero + ( val>> 1 );
}

int main()
{
  if (test (1,0) != 0)
    return 1;
  if (test (1,1) != 1)
    return 1;
  if (test (1,65535) != 1)
    return 1;
  return 0;

  return 0;
}