// expect: 55
package main;
int main() {
    /* do-while sums 1..10. Body executes at least once. */
    int s = 0;
    int i = 1;
    do {
        s = s + i;
        i = i + 1;
    } while (i <= 10);
    return s;
}
