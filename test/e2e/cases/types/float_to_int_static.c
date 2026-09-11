// expect: 0
// A floating constant initializing an integer truncates toward zero.
package main;
int x = 1.5;
int y = -2.9;
int main(void) {
    if (x != 1) return 1;
    if (y != -2) return 2;
    return 0;
}
