/* PR rtl-optimization/97386 */

// expect: 0
package main;

static unsigned char foo(unsigned int c) {
  return __builtin_bswap16((unsigned long long)(0xccccLLU << c | 0xccccLLU >> ((-c) & 63)));
}

int main(void) {
  unsigned char x = foo(0);
  if (x != 0xcc)
    __builtin_abort();
  return 0;
}
