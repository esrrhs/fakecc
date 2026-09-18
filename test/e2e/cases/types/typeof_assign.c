// expect: 0
// typeof evaluates assign / comma / increment expressions for the type.
package main;
int main(void) {
    int x = 1;
    typeof(x = 2) y = 3;
    typeof(x += 1) z = 4;
    typeof(x++) w = 5;
    typeof((x, 1.0)) d = 1.5;
    typeof(&x) p = &x;
    if (sizeof(y) != sizeof(int)) return 1;
    if (sizeof(z) != sizeof(int)) return 2;
    if (sizeof(w) != sizeof(int)) return 3;
    if (sizeof(d) != sizeof(double)) return 4;
    if (sizeof(p) != sizeof(int *)) return 5;
    if (*p != 1) return 6;
    return 0;
}
