// expect: 0
// File-scope `_Bool` initialized from an address constant or a function
// designator is 1 (the object exists).  A null pointer constant is 0.
package main;
int x;
int foo(void) { return 1; }
_Bool gp = &x;
_Bool gf = foo;
_Bool gn = 0;
int main(void) {
    if (gp != 1) return 1;
    if (gf != 1) return 2;
    if (gn != 0) return 3;
    _Bool loc = &x;
    if (loc != 1) return 4;
    return 0;
}
