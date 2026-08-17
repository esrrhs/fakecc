// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000731-2.c
package main;

int
main()
{
    int i = 1;
    int j = 0;

    while (i != 1024 || j <= 0) {
        i *= 2;
        ++ j;
    }

    if (j != 10)
      return 1;

    return 0;
}