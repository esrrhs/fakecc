// sprintf / snprintf: format into a buffer instead of writing to a file.
// Pin integer and string conversions, the return value (chars written, not
// counting the '\0'), and that snprintf clamps to the buffer size.  Then
// echo the buffer through puts so the suite's stdout check covers it.
// expect: 0
// expect_stdout: x=7
package main;
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, long n, const char *fmt, ...);
extern int puts(const char *s);
extern int strcmp(const char *a, const char *b);
int main() {
    char buf[64];
    int n;

    /* integer + string, check the exact text with strcmp */
    n = sprintf(buf, "%d=%s", 42, "hello");
    if (n != 8) return 1; /* "42=hello" is 8 chars */
    if (strcmp(buf, "42=hello") != 0) return 2;

    /* snprintf clamps: only 4 chars fit, plus the '\0' */
    n = snprintf(buf, 5, "%d", 123456);
    if (n != 6) return 3; /* would have written "123456" = 6 chars */
    if (strcmp(buf, "1234") != 0) return 4; /* clamped to 4 + '\0' */

    /* snprintf with a size that fits the whole string */
    n = snprintf(buf, 20, "x=%d", 7);
    if (n != 3) return 5;
    if (strcmp(buf, "x=7") != 0) return 6;

    /* emit via puts for the stdout assertion (puts appends '\n') */
    if (puts(buf) < 0) return 7;
    return 0;
}
