// expect: 160
// `sizeof(int[n])` is not a constant when n is a VLA dimension.  Using it as
// an array bound must keep the runtime size: n=10 → 40 ints → 160 bytes.
package main;
int main(void) {
    int n = 10;
    int a[sizeof(int[n])];
    return (int)sizeof(a);
}
