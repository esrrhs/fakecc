// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/string-1.c
package main;

int my_strlen(char *s) {
    int n = 0;
    while (*s++) n++;
    return n;
}

int main() {
    char buf[32];
    buf[0] = 'h'; buf[1] = 'e'; buf[2] = 'l';
    buf[3] = 'l'; buf[4] = 'o'; buf[5] = '\0';
    if (my_strlen(buf) != 5) return 1;
    return 0;
}
