// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991227-1.c
package main;

char* doit(int flag)
{
  return 1 + (flag ? "\0wrong\n" : "\0right\n");
}
int main()
{
  char *result = doit(0);
  if (*result == 'r' && result[1] == 'i')
    return 0;
  return 1;
}