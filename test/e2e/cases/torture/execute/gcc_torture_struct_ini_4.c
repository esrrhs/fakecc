/* struct-ini-4.c */

// expect: 0
package main;

struct s {
  int a[3];
  int c[3];
};

static struct s s = {
  c: {1, 2, 3}
};

int main(void) {
  if (s.c[0] != 1)
    __builtin_abort();
  return 0;
}
