// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000511-1.c
package main;

int f (int value, int expect)
{
  if (value != expect)
    return 1;

    return 0;}

int main()
{
  int a = 7, b = 6, c = 4, d = 7, e = 2;
	
  f (a||b%c,   1);
  f (a?b%c:0,  2);
  f (a=b%c,    2);
  f (a*=b%c,   4);
  f (a/=b%c,   2);
  f (a%=b%c,   0);
  f (a+=b%c,   2);
  f (d||c&&e,  1);
  f (d?c&&e:0, 1);
  f (d=c&&e,   1);
  f (d*=c&&e,  1);
  f (d%=c&&e,  0);
  f (d+=c&&e,  1);
  f (d-=c&&e,  0);
  f (d||c||e,  1);
  f (d?c||e:0, 0);
  f (d=c||e,   1);
  f (d*=c||e,  1);
  f (d%=c||e,  0);
  f (d+=c||e,  1);
  f (d-=c||e,  0);
  return 0;
}