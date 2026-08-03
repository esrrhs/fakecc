// expect: 42
// Empty for parts: infinite loop with break inside.
package main;
int main() {
    int i = 0;
    for (;;) {
        i = i + 1;
        if (i == 42) { break; }
    }
    return i;
}
