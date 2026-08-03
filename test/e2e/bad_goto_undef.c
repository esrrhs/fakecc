// expect_error
package main;
int main() {
    goto nowhere;   /* undeclared label */
    return 0;
}
