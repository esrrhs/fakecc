// expect: 1
// signed char -1 sign-extends to int; -1 + 2 == 1.
package main;
int main() {
    char c = 0 - 1;
    int i = c;
    return i + 2;
}
