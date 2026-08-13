// expect: 3
// static local: value persists across calls.  First call → 1, second → 2,
// third → 3.
package main;
int counter(void) {
    static int c = 0;
    c = c + 1;
    return c;
}
int main() {
    counter();
    counter();
    return counter();
}
