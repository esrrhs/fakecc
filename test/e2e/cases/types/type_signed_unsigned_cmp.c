// expect: 1
// Signed vs unsigned comparison per C §6.3.1.8:
// The -1 (int) is converted to unsigned int, becoming a huge value > 1.
package main;
int main() {
    int a = 0 - 1;
    unsigned int b = 1;
    if (a > b) { return 1; }
    return 0;
}
