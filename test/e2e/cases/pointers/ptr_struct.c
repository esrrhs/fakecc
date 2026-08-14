// Pointer arithmetic over a struct array scales by the struct size (8 here:
// two ints).  (p+n)->m must land on the n-th element's member — a scaling bug
// would read inside the wrong struct.  Mixed . and -> access through moving
// and indexed pointers, including a negative index back from a runtime base.
// expect: 0
package main;
struct S { int a; int b; };   /* 8 bytes */
int main() {
    struct S arr[4];
    arr[0].a = 1;  arr[0].b = 2;
    arr[1].a = 3;  arr[1].b = 4;
    arr[2].a = 5;  arr[2].b = 6;
    arr[3].a = 7;  arr[3].b = 8;
    struct S *p = arr;
    /* forward via -> */
    if (p->a != 1) return 1;
    if ((p + 1)->a != 3) return 2;
    if ((p + 2)->b != 6) return 3;
    if ((p + 3)->a != 7) return 4;
    /* index form is equivalent */
    if (p[0].a != 1) return 5;
    if (p[2].b != 6) return 6;
    /* runtime base, then step back */
    struct S *q = arr + 2;
    if (q->a != 5) return 7;
    if ((q - 1)->b != 4) return 8;
    if ((q - 2)->a != 1) return 9;
    if (q[-1].b != 4) return 10;
    return 0;
}
