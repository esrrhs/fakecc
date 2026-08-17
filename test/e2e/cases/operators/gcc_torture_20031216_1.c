// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20031216-1.c
package main;

/* PR optimization/13313 */
/* Origin: Mike Lerwill <mike@ml-solutions.co.uk> */

int DisplayNumber (unsigned long v)
{
  if (v != 0x9aL)
    return 1;

    return 0;}

unsigned long ReadNumber (void)
{
  return 0x009a0000L;
}

int main (void)
{
  unsigned long tmp;
  tmp = (ReadNumber() & 0x00ff0000L) >> 16;
  DisplayNumber (tmp);
  return 0;
}