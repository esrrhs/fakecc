// expect_error
// The first argument of __builtin_choose_expr must be an integer constant
// expression.  A runtime value is rejected.
package main;
int main(void) {
    int x = 1;
    return __builtin_choose_expr(x, 10, 20);
}
