// expect: 0
// A ternary mixing T* and void* has type void*, so pointer arithmetic
// strides by 1.  A null pointer constant may be 0L, not only a 32-bit 0.
package main;
int main(void) {
    int a[4];
    void *p = a;
    if ((char *)((0 ? (int *)a : p) + 1) != (char *)p + 1) return 1;
    if ((char *)((1 ? (int *)a : p) + 1) != (char *)a + 1) return 2;
    if ((0L ? p : (int *)a) != (void *)a) return 3;
    return 0;
}
