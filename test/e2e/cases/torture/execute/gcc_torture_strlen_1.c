/* strlen-1.c - simplified */

// expect: 0
package main;

int main(void) {
  char buf[64];
  int i;
  
  // Test basic strlen
  if (__builtin_strlen("hello") != 5)
    __builtin_abort();
  if (__builtin_strlen("") != 0)
    __builtin_abort();
  if (__builtin_strlen("a") != 1)
    __builtin_abort();
  
  // Test strlen with buffer
  for (i = 0; i < 10; i++)
    buf[i] = 'a';
  buf[10] = '\0';
  if (__builtin_strlen(buf) != 10)
    __builtin_abort();
  
  return 0;
}
