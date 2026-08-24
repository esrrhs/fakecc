/* PR tree-optimization/98304 */

// expect: 0
package main;

__attribute__((noipa)) int foo(int n) {
    return n - (((n > 63) ? n : 63) & -64);
}

__attribute__((noipa)) unsigned int bar(unsigned int n) {
    return n - (((n > 63) ? n : 63) & -64);
}

__attribute__((noipa)) int corge(int n) {
    return n - (((n > 1) ? n : 1) & -64);
}

__attribute__((noipa)) int thud(int n) {
    return n - (((n > 1) ? n : 1) & -64);
}

__attribute__((noipa)) int qux(int n) {
    return n - (((n > 62) ? n : 62) & -63);
}

__attribute__((noipa)) int quux(unsigned int n) {
    return n - (((n > 62) ? n : 62) & -63);
}

int main(void) {
    if (foo(-42) != -42
        || foo(0) != 0
        || foo(63) != 63
        || foo(64) != 0
        || foo(65) != 1
        || foo(99) != 35) {
            __builtin_abort();
        }
    
    if (bar(42) != 42
        || bar(0) != 0
        || bar(63) != 63
        || bar(64) != 0
        || bar(65) != 1
        || bar(99) != 35) {
            __builtin_abort();
        }

    if (corge(13) != 13
        || thud(13) != 13
        || qux(13) != 13
        || quux(13) != 13) {
            __builtin_abort();
        }

    return 0;
}
