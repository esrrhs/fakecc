// expect: 0
// Octal escape sequences: `\NNN` (char) and `\NNN` (string), up to 3 digits.
// Verifies `\101` == 'A' in a char literal, that octal escapes stop at 3
// digits in a string (`\1011` == 'A' followed by '1'), and the smallest
// values `\0` and `\7`.  Returns 0 on success, or a non-zero sentinel for the
// first failing sub-check.
package main;
int main() {
    char a = '\101';         /* 'A' = 65 */
    if (a != 65) return 1;

    /* octal escapes cap at 3 digits: `\1011` is `\101` ('A') then '1' */
    char *s = "\1011";       /* "A1" */
    if (s[0] != 65) return 2;
    if (s[1] != 49) return 3;  /* '1' */
    if (s[2] != 0) return 4;

    /* single octal digit and the smallest value */
    char c = '\0';
    char d = '\7';
    if (c != 0) return 5;
    if (d != 7) return 6;

    return 0;
}
