// expect: 0
// `_Bool ++` and `+=` convert as if by assignment: the stored value is 0 or 1,
// never 2.  Covers a simple local, a compound-assign, and a pointed-to _Bool.
package main;
int main(void) {
    _Bool b = 1;
    b++;
    if (b != 1) return 1;
    _Bool e = 1;
    e += 1;
    if (e != 1) return 2;
    _Bool z = 0;
    z++;
    if (z != 1) return 3;
    z++;
    if (z != 1) return 4;
    _Bool *p = &b;
    (*p)++;
    if (*p != 1) return 5;
    return 0;
}
