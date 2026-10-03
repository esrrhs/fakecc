// expect: 6
// A pointer parameter that stays live across a loop containing several
// va_arg extractions must not be clobbered (printf-style format walk).
// No helper calls inside the loop: only the inline va_arg sequence may
// touch scratch regs, so a mistaken home in R11 would show up here.
package main;
long vf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    long acc = 0;
    while (*fmt) {
        if (*fmt == '%') {
            ++fmt;
            acc += (long)va_arg(ap, long);
        }
        ++fmt;
    }
    return acc;
}
int main(void) { return (int)vf("%d%d%d", 1, 2, 3); }
