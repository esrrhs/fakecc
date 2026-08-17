// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/980505-1.c
package main;

static int f(int) ;
int main()
{
   int f1, f2, x;
   x = 1; f1 = f(x);
   x = 2; f2 = f(x);
   if (f1 != 1 || f2 != 2)
     return 1;
   return 0;
}
static int f(int x) { return x; }