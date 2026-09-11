// expect: 0
// Signed __int128 right shift in a static initializer is arithmetic, matching
// the runtime shift: ((__int128)-1) >> 1 stays -1.
package main;
__int128 x = ((__int128)-1) >> 1;
int main(void) {
    if (x != (__int128)-1) return 1;
    return 0;
}
