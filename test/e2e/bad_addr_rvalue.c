// expect_error
// Cannot take address of a non-lvalue.
package main;
int main() {
    int *p = &(1 + 2);
    return 0;
}
