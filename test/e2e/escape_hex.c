// expect: 0
// Hexadecimal escape sequences: `\xHH` (char) and `\xHH...` (string).
// Verifies `\x41` == 'A' in a char literal, that `\x` is greedy (consumes all
// consecutive hex digits, so `\x42C` == 0x42C low byte == 0x2C), and that
// separate hex escapes decode independently in a string.  Returns 0 on
// success, or a non-zero sentinel for the first failing sub-check.
package main;
int main() {
    char ch = '\x41';        /* 'A' = 65 */
    if (ch != 65) return 1;

    /* `\x` is greedy: `\x42C` is the single value 0x42C, low byte 0x2C = 44 */
    char g = '\x42C';
    if (g != 44) return 2;

    /* two adjacent hex escapes in a string decode independently */
    char *s = "\x41\x42";    /* "AB" */
    if (s[0] != 65) return 3;
    if (s[1] != 66) return 4;
    if (s[2] != 0) return 5;

    /* the escapes already checked above cover the string path; this anchors
     * the char path one more time with a different value */
    if ('\x0a' != 10) return 6;

    return 0;
}
