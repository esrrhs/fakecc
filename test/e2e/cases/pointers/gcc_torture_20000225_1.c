// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000225-1.c
package main;

int main ()
{
    int nResult;
    int b=0;
    int i = -1;

    do
    {
     if (b!=0) {
       return 1;
       nResult=1;
     } else {
      nResult=0;
     }
     i++;
     b=(i+2)*4;
    } while (i < 0);
    return 0;
}