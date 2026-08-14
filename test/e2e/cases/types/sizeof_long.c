// sizeof(long) and sizeof(long long): both are 8 bytes on x86-64 SysV.
// The compiler folds sizeof into a constant, so the result is just the
// integer value.  Pin both, plus a long-long arithmetic that only works
// if the type is wide enough.
// expect: 0
package main;
int main() {
    if ((int)sizeof(long) != 8) return 1;
    if ((int)sizeof(long long) != 8) return 2;

    /* 3000000000 fits in 64-bit but not 32-bit; forces real wide math */
    long big = 3000000000L;
    if (big <= 0) return 3; /* would wrap to negative if truncated to 32-bit */
    if (big + big != 6000000000L) return 4;

    long long bigger = 6000000000LL;
    if (bigger / 2 != 3000000000LL) return 5;

    return 0;
}
