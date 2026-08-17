// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/bitfld-2.c
package main;

/* Test whether bit field boundaries aren't advanced if bit field type
   has alignment large enough.  */

struct A {
  unsigned short a : 5;
  unsigned short b : 5;
  unsigned short c : 6;
};

struct B {
  unsigned short a : 5;
  unsigned short b : 3;
  unsigned short c : 8;
};

int main ()
{
  /* If short is not at least 16 bits wide, don't test anything.  */
  if ((unsigned short) 65521 != 65521)
    return 0;

  if (sizeof (struct A) != sizeof (struct B))
    return 1;

  return 0;
}