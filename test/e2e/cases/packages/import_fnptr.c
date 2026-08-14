// expect: 0
// expect_stdout: hi
// expect_stdout: hi
// expect_stdout: hi
/* Imported functions used as values (callback / &fn), not only as calls. */
package main;
import runtime;
int call1(int (*fp)(const char *), const char *s) {
    return fp(s);
}
int main(void) {
    int (*p)(const char *);
    if (call1(runtime.puts, "hi") < 0) return 1;
    p = runtime.puts;
    if (p("hi") < 0) return 2;
    if ((&runtime.puts)("hi") < 0) return 3;
    return 0;
}
