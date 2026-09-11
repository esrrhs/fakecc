// expect: 0
// Logical && / || are integer constant expressions.
package main;
int x = 1 && 2;
int y = 0 && 1;
int z = 0 || 5;
int main(void) {
    if (x != 1) return 1;
    if (y != 0) return 2;
    if (z != 1) return 3;
    return 0;
}
