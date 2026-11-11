// expect: 0
// `T *p = &v` must keep a usable pointer at -O0 and -O1 (not only folded
// *p).  The ADDR-copy fold for compound-literal temps must not drop this lea
// in a way that breaks later pointer uses.
package main;

int take(int *p, int *q) {
    return *p + *q;
}

int main(void) {
    int v = 7;
    int w = 9;
    int *p = &v;
    int *q = &w;
    if (*p != 7) return 1;
    *p = *p + 1;
    if (v != 8) return 2;
    if (take(p, q) != 17) return 3;
    if (p == q) return 4;
    return 0;
}
