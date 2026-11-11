// expect: 0
/* Folded frame vadd/vxor (no dest-ADDR lea) must keep C semantics. */
package main;

typedef int v4si __attribute__((vector_size(16)));

int main(void) {
    v4si a, b, c;
    unsigned acc;
    int i;

    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;
    b[0] = 5; b[1] = 6; b[2] = 7; b[3] = 8;
    acc = 0;
    for (i = 0; i < 16; i++) {
        c = a + b;
        a = c ^ b;
        b[i & 3] += (int)(i & 15);
        acc += (unsigned)a[0] + (unsigned)a[1] + (unsigned)a[2] + (unsigned)a[3];
    }
    return acc == 4688u ? 0 : 1;
}
