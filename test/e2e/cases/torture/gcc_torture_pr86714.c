/* PR tree-optimization/86714 */

// expect: 0
package main;

char a[2][3] = { "123", "xyz" };
char b[6];

void *pb = b;

int main (void)
{
   __builtin_memcpy (b, a, 4);
   __builtin_memset (b + 4, 'a', 2);

   if (b[0] != '1' || b[1] != '2' || b[2] != '3'
       || b[3] != 'x' || b[4] != 'a' || b[5] != 'a')
     __builtin_abort ();

   return 0;
}
