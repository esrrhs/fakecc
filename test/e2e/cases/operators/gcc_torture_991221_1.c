// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/991221-1.c
package main;

int main( void )
{
   unsigned long totalsize = 80;
   unsigned long msize = 64;

   if (sizeof(long) != 4)
     return 0;
   
   if ( totalsize > (2147483647L   * 2UL + 1)  
        || (msize != 0 && ((msize - 1) > (2147483647L   * 2UL + 1) )))
      return 1;
   return 0;
}