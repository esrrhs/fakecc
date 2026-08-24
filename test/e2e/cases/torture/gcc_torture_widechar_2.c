/* widechar-2.c */

// expect: 0
package main;

static const wchar_t ws[] = { 'f', 'o', 'o', 0 };

int main(void) {
  if (ws[0] != 'f' || ws[1] != 'o' || ws[2] != 'o' || ws[3] != 0)
    __builtin_abort();
  return 0;
}
