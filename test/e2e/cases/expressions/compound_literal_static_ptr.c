// expect: 0
// File-scope pointer initialized from a compound-literal array must decay to
// the address of a static object, not pack the first element into the pointer
// slot (that used to SIGSEGV on dereference).
package main;
static int *p = (int[]){1, 2, 3};
static int *q = (int[]){4, 5, 6};
int main(void) {
    if (p[0] != 1) return 1;
    if (p[1] != 2) return 2;
    if (p[2] != 3) return 3;
    if (q[0] != 4) return 4;
    if (p == q) return 5;
    int *r = (int[]){7, 8, 9};
    if (r[0] != 7) return 6;
    if (r[1] != 8) return 7;
    if (r[2] != 9) return 8;
    return 0;
}
