// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/980505-2.c
package main;

typedef unsigned short Uint16;
typedef unsigned int Uint;

Uint f ()
{
        Uint16 token;
        Uint count;
        static Uint16 values[1] = {0x9300};

        token = values[0];
        count = token >> 8;

        return count;
}

int
main ()
{
  if (f () != 0x93)
    return 1;
  return 0;
}