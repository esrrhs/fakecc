// Incremental pointer walk: instead of indexing a[i], advance a pointer p++.
// Codegen must post-increment by the pointed-to size (4 for int).  This is the
// pattern a scaling bug breaks silently — p++ that adds 1 instead of 4 reads
// each int's bytes spread across three subsequent "elements".  Sum both ways
// and require they agree.
// expect: 0
package main;
int main() {
    int a[6];
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4; a[4] = 5; a[5] = 6;
    int sum_idx = 0;
    int i = 0;
    for (; i < 6; i = i + 1) { sum_idx = sum_idx + a[i]; }
    int sum_walk = 0;
    int *p = a;
    int *end = a + 6;
    while (p < end) {
        sum_walk = sum_walk + *p;
        p = p + 1;
    }
    if (sum_idx != 21) return 1;
    if (sum_walk != 21) return 2;
    if (sum_walk != sum_idx) return 3;
    return 0;
}
