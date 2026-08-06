// expect: 5
// Float stored in a struct member, then loaded back and used.  Before the
// fix, a float member loaded through a pointer (IR_LOAD_PTR) was treated as
// an integer and landed in a GPR, so the value read back was garbage (0).
package main;
struct F { int kind; float val; };
int main(void) {
    struct F a;
    a.kind = 1;
    a.val = 2.5f;
    struct F b;
    b = a;
    return (int)(b.val * 2.0f);
}
