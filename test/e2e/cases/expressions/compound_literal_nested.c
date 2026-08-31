// expect: 0
package main;

/* Nested struct member: `&((Outer){...}).inner.field`.  The literal is static
 * storage; the slot holds the address of the nested member. */
struct Inner { int a; int b; };
struct Outer { int tag; struct Inner inner; int tail; };

static int *pb = &((struct Outer){ 1, { 2, 3 }, 4 }).inner.b;
static struct Outer *po = &((struct Outer){ 5, { 6, 7 }, 8 });

int main(void) {
    if (*pb != 3) return 1;
    if (po->tag != 5) return 2;
    if (po->inner.a != 6) return 3;
    if (po->inner.b != 7) return 4;
    if (po->tail != 8) return 5;
    return 0;
}
