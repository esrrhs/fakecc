// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20100209-1.c
package main;

int bar(int foo)
{
  return (int)(((unsigned long long)(long long)foo) / 8);
}

int main()
{
  if (sizeof (long long) > sizeof (int)
      && bar(-1) != -1)
    return 1;
  return 0;
}