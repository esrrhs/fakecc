/* strcpy-1.c - simplified */

// expect: 0
package main;

int main(void) {
  char buf1[64];
  char buf2[64];
  int off1, off2, len, i;
  char *q;

  for (off1 = 0; off1 < 4; off1++)
    for (off2 = 0; off2 < 4; off2++)
      for (len = 1; len < 10; len++) {
        for (i = 0; i < 64; i++) {
          buf1[i] = 'a';
          buf2[i] = 'A' + (i % 26);
        }
        buf2[off2 + len] = '\0';

        __builtin_strcpy(buf1 + off1, buf2 + off2);

        q = buf1;
        for (i = 0; i < off1; i++, q++)
          if (*q != 'a')
            __builtin_abort();

        for (i = 0; i < len; i++, q++)
          if (*q != buf2[off2 + i])
            __builtin_abort();

        if (*q++ != '\0')
          __builtin_abort();
        for (i = 0; i < 4; i++, q++)
          if (*q != 'a')
            __builtin_abort();
      }

  return 0;
}
