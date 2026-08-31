// expect: 0
package main;

/* Static-storage semantics: each file-scope compound literal is a distinct,
 * writable, static object.  Two literals of the same type must have different
 * addresses, and mutations through one pointer must not affect the other. */
static int *p1 = &(int){1};
static int *p2 = &(int){2};

int main(void) {
    if (p1 == p2) return 1;     /* distinct objects */
    if (*p1 != 1) return 2;
    if (*p2 != 2) return 3;
    *p1 = 100;
    if (*p2 != 2) return 4;     /* writing p1 must not clobber p2 */
    if (*p1 != 100) return 5;
    return 0;
}
