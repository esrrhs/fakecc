// expect: 0
// Allocate with runtime.malloc, write through the pointer, runtime.free, and
// verify the bytes we wrote.
package main;
import runtime;
int main(void) {
    int *p = (int *)runtime.malloc(8);
    if (p == 0) return 1;
    p[0] = 11;
    p[1] = 22;
    if (p[0] != 11 || p[1] != 22) { runtime.free(p); return 2; }
    runtime.free(p);
    return 0;
}
