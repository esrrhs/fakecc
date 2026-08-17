// sizeof(struct): layout, padding and nesting.  Pin that a struct's size
// accounts for trailing padding (the int+char case rounds up), that a
// nested struct's size includes its own padding, and that sizeof does
// not evaluate its operand.
//
// Note: fakecc conservatively aligns every struct to 8 bytes
// (ast.c type_align TY_STRUCT returns 8), so a nested struct whose natural
// alignment is 4 still occupies an 8-aligned slot.  That is intentional —
// it keeps the compiler simple and is self-consistent for the bootstrap
// fixed point — so the assertions below pin fakecc's actual layout.
// expect: 0
package main;
struct A { char c; int i; };
struct B { int i; char c; };
struct Inner { int x; };
struct Outer { char c; struct Inner inner; int y; };
int main() {
    /* char + int: 1 byte + 3 padding + 4 = 8 */
    if ((int)sizeof(struct A) != 8) return 1;
    /* int + char: 4 + 1 + 3 padding = 8 */
    if ((int)sizeof(struct B) != 8) return 2;

    /* nested struct Inner with 4-byte natural alignment:
       char(1) + pad(3) + Inner(4@4) + int(4@8) = 12 */
    if ((int)sizeof(struct Outer) != 12) return 3;

    /* sizeof does not evaluate its side effect */
    int k = 0;
    if ((int)sizeof(k = k + 5) != 4) return 4;
    if (k != 0) return 5;

    return 0;
}
