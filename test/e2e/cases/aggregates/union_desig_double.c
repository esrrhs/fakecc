// expect: 0
// Designated union initialization writes the named member, not member 0.
package main;
union U { int i; double d; };
union U g = { .d = 1.0 };
int main(void) {
    union U u = { .d = 1.0 };
    if (u.d != 1.0) return 1;
    if (g.d != 1.0) return 2;
    return 0;
}
