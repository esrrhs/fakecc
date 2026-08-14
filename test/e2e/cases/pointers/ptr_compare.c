// Pointer comparison operators (<, >, <=, >=, ==, !=).  Addresses into the same
// array are ordered by index; pointers to distinct objects compare but without
// a defined ordering, so the suite only compares within one array.  The
// interesting codegen cases are <= and >=, which must accept equality, and
// the != / == pair, which codegen lowers to a plain integer compare of the
// addresses.
// expect: 0
package main;
int main() {
    int a[5];
    int *p = a;          /* &a[0] */
    int *q = a + 2;      /* &a[2] */
    int *r = a + 2;      /* also &a[2] */
    /* strict ordering */
    if (!(p < q))  return 1;
    if (!(q > p))  return 2;
    if (p > q)     return 3;
    if (q < p)     return 4;
    /* non-strict: <= and >= accept equality */
    if (!(p <= q)) return 5;
    if (!(q >= p)) return 6;
    if (!(p <= p)) return 7;   /* equal pointers */
    if (!(q >= q)) return 8;
    if (!(q <= r)) return 9;   /* q == r */
    if (!(q >= r)) return 10;
    /* equality / inequality */
    if (p == q)    return 11;
    if (!(p != q)) return 12;
    if (!(q == r)) return 13;
    if (q != r)    return 14;
    return 0;
}
