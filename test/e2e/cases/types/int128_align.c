// expect: 0
package main;

__int128 take(__int128 a, __int128 b) { return a + b; }

int main(void) {
    __int128 x = 1;
    __int128 y = 2;
    if ((int)take(x, y) != 3) return 1;
    long double ld = 1.0L;
    if ((int)ld != 1) return 2;
    return 0;
}
