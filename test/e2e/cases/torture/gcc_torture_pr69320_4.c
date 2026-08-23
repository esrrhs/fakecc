// expect: 0
package main;

extern void abort(void);
extern void exit(int);

int a;
char b, d;
short c;

short fn1(int p1, int p2) { return p2 >= 2 ? p1 : p1 > p2; }

int main(void) {
  int *e = &a, *f = &a;
  b = 1;
  for (; b <= 9; b++) {
    c = *e != 5 || d;
    *f = fn1(c || b, a);
  }
  if ((long long) a != 1)
    abort ();
  exit (0);
}
