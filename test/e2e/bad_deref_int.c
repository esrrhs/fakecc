// expect_error
// Cannot dereference a plain int.
package main;
int main() {
    int x = 5;
    int y = *x;
    return y;
}
