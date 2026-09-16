// expect: 0
// Compound-assign of __int128 must evaluate the lvalue once: a[i++] += k
// increments i once and writes the first slot, not the second.
package main;
int main(void) {
    __int128 a[2];
    int i;
    a[0] = 10;
    a[1] = 20;
    i = 0;
    a[i++] += 3;
    if (i != 1) return 1;
    if ((int)a[0] != 13) return 2;
    if ((int)a[1] != 20) return 3;
    a[i++] += 5;
    if (i != 2) return 4;
    if ((int)a[1] != 25) return 5;
    return 0;
}
