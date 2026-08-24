/* pr58419.c */

// expect: 0
package main;

static int a, g, i, k, *p;
static signed char b;
static char e;
static short c, h;
static short *d = &c;

static char foo(int p1, int p2) {
  return p1 - p2;
}

static void dummy(void) {
}

static int bar(void) {
  short *q = &c;
  *q = 1;
  *p = 0;
  return 0;
}

int main(void) {
  for (b = -22; b >= -29; b--) {
    short *l = &h;
    char *m = &e;
    *l = a;
    g = foo(*m = k && *d, 1 > i) || bar();
  }
  dummy();
  return 0;
}
