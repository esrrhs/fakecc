// expect: 0
package main;

extern void abort(void);
extern void exit(int);

int a, b, d, f;
char c;
static int *e = &d;

int main(void) {
  int g = -1;
  *e = g;
  c = 4;
  for (; c >= 14; c++)
    *e = 1;
  f = a == 0;
  *e ^= f;
  int h = ~d;
  if (d)
    b = h;
  if (h)
    exit (0);
  abort ();
}
