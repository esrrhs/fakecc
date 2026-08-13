// expect: 42
package main;
int main() {
    int a = 3;
    int b = 0;
    /* Ternary used as the controlling expression of an if. */
    if ((a ? b : 42)) {
        return 1;
    }
    return 42;
}
