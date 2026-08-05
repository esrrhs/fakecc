// Float comparisons: (2.5 > 1.5) is true (1), plus 41 = 42.
// expect: 42
package main;
int main() {
    double a = 2.5;
    double b = 1.5;
    if (a > b) return 42;
    return 0;
}
