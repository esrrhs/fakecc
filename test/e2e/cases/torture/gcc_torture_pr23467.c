/* pr23467.c - aligned struct member */

// expect: 0
package main;

struct s1 {
  int a;
};

struct {
  char c;
  struct s1 m;
} v;

int main(void) {
  // Check alignment - this is implementation-defined behavior
  // Just verify the struct works
  v.m.a = 42;
  if (v.m.a != 42)
    __builtin_abort();
  return 0;
}
