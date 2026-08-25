/* PR middle-end/87053 */

// expect: 0
package main;

static const union {
  struct {
    char x[4];
    char y[4];
  };
  struct {
    char z[8];
  };
} u = {{"1234", "567"}};

int main(void) {
  if (__builtin_strlen(u.z) != 7)
    __builtin_abort();
  return 0;
}
