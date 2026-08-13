// expect: 55
package main;
int sum_to(int n) {
    int s = 0;
    while (n > 0) { s = s + n; n = n - 1; }
    return s;
}
int square(int x) { return x * x; }
int main() {
    int a = sum_to(10);
    int b = square(3);
    if (a == 55) {
        if (b == 9) { return a; }
    }
    return 0;
}
