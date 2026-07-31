// expect: 120
package main;
int main() {
    int n = 5;
    int r = 1;
    while (n > 1) {
        r = r * n;
        n = n - 1;
    }
    return r;
}
