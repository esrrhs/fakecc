// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/990106-2.c
package main;

unsigned calc_mp(unsigned mod)
{
      unsigned a,b,c;
      c=-1;
      a=c/mod;
      b=0-a*mod;
      if (b > mod) { a += 1; b-=mod; }
      return b;
}

int main(int argc, char *argv[])
{
      unsigned x = 1234;
      unsigned y = calc_mp(x);

      if ((sizeof (y) == 4 && y != 680)
	  || (sizeof (y) == 2 && y != 134))
	return 1;
      return 0;
}