// expect: 0
package main;

/* Empty struct compound literal: `&(struct A){}`.  The literal still gets
 * static storage and a valid address (pr87647.c regression shape). */
struct A {};

static struct A *const b = &(struct A){};

int main(void) {
    /* The pointer must be non-NULL (it has static storage). */
    if (b == (struct A *)0) return 1;
    return 0;
}
