// Pointer subtraction yields the *element* count between two pointers into the
// same array, not the byte count.  For int* the raw byte distance is scaled
// down by sizeof(int)==4; for a struct S (8 bytes) it scales by 8.  The codegen
// must divide the byte delta by the pointed-to size — forgetting that division
// returns a byte count that is off by the type-size factor.
// expect: 0
package main;
struct S { int a; int b; };  /* 8 bytes */
int main() {
    int a[6];
    int *p = a;
    int *q = a + 5;
    if (q - p != 5) return 1;
    if (p - q != -5) return 2;
    /* same pointer: difference is zero */
    if (p - p != 0) return 3;
    /* struct scaling: two elements are 16 bytes apart but the diff is 2 */
    struct S arr[4];
    struct S *sp = arr;
    struct S *sq = arr + 3;
    if (sq - sp != 3) return 4;
    /* char scaling: byte distance == element distance */
    char c[10];
    char *cp = c;
    char *cq = c + 7;
    if (cq - cp != 7) return 5;
    return 0;
}
