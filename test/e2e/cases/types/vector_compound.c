// expect: 0
package main;

typedef int v4si __attribute__((vector_size(16)));

int main(void) {
    v4si a = {1, 2, 3, 4};
    v4si b = {5, 6, 7, 8};
    a += b;
    if (a[0] != 6 || a[3] != 12) return 1;
    a -= b;
    if (a[0] != 1) return 2;
    return 0;
}
