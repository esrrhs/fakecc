// fmt.sprintf / fmt.snprintf: format into a buffer instead of writing to a file.
// Pin integer and string conversions, the return value (chars written, not
// counting the '\0'), and that fmt.snprintf clamps to the buffer size.  Then
// echo the buffer through io.puts so the suite's io.stdout check covers it.
// expect: 0
// expect_stdout: x=7
package main;
import fmt;
import io;
import str;
int main() {
    char buf[64];
    int n;

    /* integer + string, check the exact text with str.strcmp */
    n = fmt.sprintf(buf, "%d=%s", 42, "hello");
    if (n != 8) return 1; /* "42=hello" is 8 chars */
    if (str.strcmp(buf, "42=hello") != 0) return 2;

    /* fmt.snprintf clamps: only 4 chars fit, plus the '\0' */
    n = fmt.snprintf(buf, 5, "%d", 123456);
    if (n != 6) return 3; /* would have written "123456" = 6 chars */
    if (str.strcmp(buf, "1234") != 0) return 4; /* clamped to 4 + '\0' */

    /* fmt.snprintf with a size that fits the whole string */
    n = fmt.snprintf(buf, 20, "x=%d", 7);
    if (n != 3) return 5;
    if (str.strcmp(buf, "x=7") != 0) return 6;

    /* emit via io.puts for the io.stdout assertion (io.puts appends '\n') */
    if (io.puts(buf) < 0) return 7;
    return 0;
}
