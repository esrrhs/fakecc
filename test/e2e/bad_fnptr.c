// expect_error
// Calling through an `int*` (not a function pointer) must be rejected.
package main;
int main() {
    int *p;
    return p(3);
}
