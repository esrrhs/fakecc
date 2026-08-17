// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990531-1.c
package main;

unsigned long bad(int reg, unsigned long inWord)
   {
       union {
           unsigned long word;
           unsigned char byte[4];
       } data;

       data.word = inWord;
       data.byte[reg] = 0;

       return data.word;
   }

int
main(void)
{
  /* XXX This test could be generalized.  */
  if (sizeof (long) != 4)
    return 0;

  if (bad (0, 0xdeadbeef) == 0xdeadbeef)
    return 1;
  return 0;
}