// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020201-1.c
package main;

/* Test whether division by constant works properly.  */

unsigned char cx = 7;
unsigned short sx = 14;
unsigned int ix = 21;
unsigned long lx = 28;
unsigned long long Lx = 35;

int
main ()
{
  unsigned char cy;
  unsigned short sy;
  unsigned int iy;
  unsigned long ly;
  unsigned long long Ly;
  
  cy = cx / 6; if (cy != 1) return 1;
  cy = cx % 6; if (cy != 1) return 1;

  sy = sx / 6; if (sy != 2) return 1;
  sy = sx % 6; if (sy != 2) return 1;

  iy = ix / 6; if (iy != 3) return 1;
  iy = ix % 6; if (iy != 3) return 1;

  ly = lx / 6; if (ly != 4) return 1;
  ly = lx % 6; if (ly != 4) return 1;

  Ly = Lx / 6; if (Ly != 5) return 1;
  Ly = Lx % 6; if (Ly != 5) return 1;

  return 0;
}