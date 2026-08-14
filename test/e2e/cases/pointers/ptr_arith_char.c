// char pointer arithmetic scales by sizeof(char) == 1, so p+n advances byte
// by byte.  This is the one case where the raw address and the scaled address
// coincide — easy to get right by accident while wider types are broken.
// Walking a C string by cp++ and reading through the moving pointer.
// expect: 0
package main;
int main() {
    char s[6];
    s[0] = 'H'; s[1] = 'e'; s[2] = 'l'; s[3] = 'l'; s[4] = 'o'; s[5] = 0;
    char *cp = s;
    if (*(cp + 0) != 'H') return 1;
    if (*(cp + 1) != 'e') return 2;
    if (*(cp + 4) != 'o') return 3;
    /* advance and read back */
    cp = cp + 2;
    if (*cp != 'l') return 4;
    if (*(cp - 1) != 'e') return 5;
    if (*(cp + 2) != 'o') return 6;
    return 0;
}
