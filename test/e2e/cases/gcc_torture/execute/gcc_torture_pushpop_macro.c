// expect: 0
package main;
extern void abort ();
       
       
int main ()
{
  if (2 != 2)
    abort ();
  return 0;
}
