// expect: 0
// SysV / C11 max_align_t is 16 bytes.  A 24-byte chunk header left the
// user pointer at 8 mod 16; SSE movaps into a malloc'd object #GPs.
package main;
import runtime;
int main(void) {
    void *p;
    int i;
    i = 0;
    while (i < 8) {
        p = runtime.malloc(1 + (unsigned)i);
        if (p == 0) return 1;
        if (((unsigned long)p & 15ul) != 0) return 2;
        runtime.free(p);
        i = i + 1;
    }
    p = runtime.malloc(64);
    if (p == 0) return 3;
    if (((unsigned long)p & 15ul) != 0) return 4;
    runtime.free(p);
    return 0;
}
