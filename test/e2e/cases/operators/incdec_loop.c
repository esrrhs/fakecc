// expect: 10
package main;
int main() {
    /* Classic while-loop with postfix ++: sum 0..4 = 10. */
    int i = 0;
    int s = 0;
    while (i < 5) {
        s = s + i;
        i++;
    }
    return s;
}
