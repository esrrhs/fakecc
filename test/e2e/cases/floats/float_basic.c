// Float variable + arithmetic: 1.5 + 1.5 = 3.0 → int 3.
// expect: 3
package main;
int main() {
    float a = 1.5;
    float b = 1.5;
    return (int)(a + b);
}
