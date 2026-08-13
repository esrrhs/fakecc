// expect: 30
// Array-to-pointer decay + pointer arithmetic.
package main;
int main() {
    int a[5];
    a[0] = 10; a[1] = 20; a[2] = 30; a[3] = 40; a[4] = 50;
    int *p = a;
    return *(p + 2);
}
