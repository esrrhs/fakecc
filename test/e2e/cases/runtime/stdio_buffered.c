// runtime.sprintf / runtime.snprintf: format into a buffer instead of writing to a file.
// Pin integer and string conversions, the return value (chars written, not
// counting the '\0'), and that runtime.snprintf clamps to the buffer size.  Then
// echo the buffer through runtime.puts so the suite's runtime.stdout check covers it.
// expect: 0
// expect_stdout: x=7
package main;
import runtime;
int main() {
    char buf[64];
    int n;

    /* integer + string, check the exact text with runtime.strcmp */
    n = runtime.sprintf(buf, "%d=%s", 42, "hello");
    if (n != 8) return 1; /* "42=hello" is 8 chars */
    if (runtime.strcmp(buf, "42=hello") != 0) return 2;

    /* runtime.snprintf clamps: only 4 chars fit, plus the '\0' */
    n = runtime.snprintf(buf, 5, "%d", 123456);
    if (n != 6) return 3; /* would have written "123456" = 6 chars */
    if (runtime.strcmp(buf, "1234") != 0) return 4; /* clamped to 4 + '\0' */

    /* runtime.snprintf with a size that fits the whole string */
    n = runtime.snprintf(buf, 20, "x=%d", 7);
    if (n != 3) return 5;
    if (runtime.strcmp(buf, "x=7") != 0) return 6;

    /* emit via runtime.puts for the runtime.stdout assertion (runtime.puts appends '\n') */
    if (runtime.puts(buf) < 0) return 7;
    return 0;
}
