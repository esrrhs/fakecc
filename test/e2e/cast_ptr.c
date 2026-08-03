// expect: 1
// Pointer-to-long cast: any live stack address is non-zero.
package main;
int main() {
    int x = 7;
    long a = (long)&x;
    if (a == 0) { return 0; }
    return 1;
}
