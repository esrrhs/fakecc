// expect: 0
package main;
int main() {
    int x = 42;
    int a = x & 0;
    int b = 0 & x;
    int c = x | 0;
    int d = 0 | x;
    int e = x ^ 0;
    int f = 0 ^ x;
    /* a=0, b=0, c=42, d=42, e=42, f=42 */
    /* (0 + 0 + 42 + 42 + 42 + 42) - 168 = 0 */
    return (a + b + c + d + e + f) - 168;
}
