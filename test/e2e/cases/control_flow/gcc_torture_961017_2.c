// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/961017-2.c
package main;

int
main (void)
{
  int i = 0;

  if (sizeof (unsigned long int) == 4)
    {
      unsigned long int z = 0;

      do {
	z -= 0x00004000;
	i++;
	if (i > 0x00040000)
	  return 1;
      } while (z > 0);
      return 0;
    }
  else if (sizeof (unsigned int) == 4)
    {
      unsigned int z = 0;

      do {
	z -= 0x00004000;
	i++;
	if (i > 0x00040000)
	  return 1;
      } while (z > 0);
      return 0;
    }
  else
    return 0;
}