// expect: 0
// Allocate with mem.malloc, write through the pointer, mem.free, and
// verify the bytes we wrote.
package main;
import mem;
int main(void) {
    int *p = (int *)mem.malloc(8);
    if (p == 0) return 1;
    p[0] = 11;
    p[1] = 22;
    if (p[0] != 11 || p[1] != 22) { mem.free(p); return 2; }
    mem.free(p);
    return 0;
}
