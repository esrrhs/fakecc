// expect: 7
// Typedef for a struct type: define the struct, then alias it, then declare
// a variable via the typedef name.
package main;
struct S { int a; int b; };
typedef struct S S;
int main() {
    S s;
    s.a = 3;
    s.b = 4;
    return s.a + s.b;
}
