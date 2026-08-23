/* PR target/98681 */

// expect: 0
package main;

static int foo(int x) {
  if (x > 32)
    return (x << -64) & 255;
  else
    return x;
}

int main(void) {
  if (foo(32) != 32 || foo(-150) != -150)
    __builtin_abort();
  return 0;
}
