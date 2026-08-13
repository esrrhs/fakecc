// expect: 10
package main;
int main() {
    /* Comma operator: left operand evaluated for side effect (x = 3),
     * result is the right operand (x + 7 == 10). */
    int x = 0;
    return (x = 3, x + 7);
}
