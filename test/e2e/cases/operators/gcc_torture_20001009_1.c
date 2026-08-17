// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20001009-1.c
package main;

int a,b;
int
main(void)
{
  int c=-2;
  int d=0xfe;
  int e=a&1;
  int f=b&2;
  if ((char)(c|(e&f)) == (char)d)
    return 0;
  else
    return 1;
}