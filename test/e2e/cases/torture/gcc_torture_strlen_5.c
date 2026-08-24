/* strlen-5.c - simplified */

// expect: 0
package main;

int main(void) {
  // Test strlen with various offsets
  char buf[32];
  int i;
  
  // Fill with 'a's
  for (i = 0; i < 16; i++)
    buf[i] = 'a';
  buf[16] = '\0';
  
  // Test from different offsets
  if (__builtin_strlen(buf) != 16)
    __builtin_abort();
  if (__builtin_strlen(buf + 4) != 12)
    __builtin_abort();
  if (__builtin_strlen(buf + 8) != 8)
    __builtin_abort();
  if (__builtin_strlen(buf + 16) != 0)
    __builtin_abort();
  
  return 0;
}
