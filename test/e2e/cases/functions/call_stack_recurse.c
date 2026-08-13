// expect: 45
// Recursion with stack args — factorial-like accumulator with 8 params.
package main;
int helper(int acc, int n, int p1, int p2, int p3, int p4, int p5, int p6) {
    if (n == 0) { return acc + p1 + p2 + p3 + p4 + p5 + p6; }
    return helper(acc + n, n - 1, p1, p2, p3, p4, p5, p6);
}
int main() {
    return helper(0, 9, 0, 0, 0, 0, 0, 0);   // 0+9+8+...+1 = 45
}
