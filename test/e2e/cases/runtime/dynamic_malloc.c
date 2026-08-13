// expect: 0
// Allocate with extern libc malloc, write through the pointer, free, and
// verify the bytes we wrote.  Exercises the PLT with pointer args + return.
package main;
extern void *malloc(long n);
extern void free(void *p);
int main(void) {
    int *p = (int *)malloc(8);
    if (p == 0) return 1;
    p[0] = 11;
    p[1] = 22;
    if (p[0] != 11 || p[1] != 22) { free(p); return 2; }
    free(p);
    return 0;
}
