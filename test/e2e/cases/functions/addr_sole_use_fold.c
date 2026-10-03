// expect: 0
// A single-consumer address (plain p[i] read / write) must still behave
// correctly when folded into [base+index*scale].
package main;
long rd(long *p, int i) { return p[i]; }
void wr(long *p, int i, long v) { p[i] = v; }
int main(void) {
    long a[6] = {1, 2, 3, 4, 5, 6};
    if (rd(a, 4) != 5) return 1;
    wr(a, 2, 99);
    if (a[2] != 99 || a[1] != 2 || a[3] != 4) return 2;
    return 0;
}
