// expect: 1
package main;
int main() {
    /* Body executes exactly once even though condition is false. */
    int x = 0;
    do {
        x = x + 1;
    } while (0);
    return x;
}
