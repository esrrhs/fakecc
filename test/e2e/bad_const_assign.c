// expect_error
package main;
int main() {
    const int x = 5;
    x = 10;   /* assigning to a const variable must be rejected */
    return x;
}
