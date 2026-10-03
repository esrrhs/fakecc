// expect: 0
// a[i]++ : the element address feeds both the load and the store.  The
// addressing-mode fold must not apply the index twice to such a shared
// address (it may only fold an address with a single consumer).
package main;
long g[8];
long bump(long *p, int i) {
    p[i]++;
    return p[i];
}
int main(void) {
    long a[8];
    for (int k = 0; k < 8; k++) a[k] = k * 10;
    if (bump(a, 3) != 31) return 1;
    if (a[3] != 31 || a[2] != 20 || a[4] != 40) return 2;
    g[5] = 7;
    g[5]++;
    if (g[5] != 8 || g[4] != 0 || g[6] != 0) return 3;
    long *q = &a[1];
    *q++ = *q + 1;
    if (a[1] != 11 || q != &a[2]) return 4;
    return 0;
}
