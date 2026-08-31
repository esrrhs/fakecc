// expect: 0
package main;

/* `&((T){...}).member` — address of a member of a file-scope compound literal.
 * The literal gets static storage; the slot holds the address of member `y`
 * (offset 4).  Reading through it must yield the initialized value. */
struct S { int x; int y; };

static int *py = &((struct S){10, 20}).y;
static struct S *ps = &((struct S){30, 40});

int main(void) {
    if (*py != 20) return 1;
    if (ps->x != 30) return 2;
    if (ps->y != 40) return 3;
    return 0;
}
