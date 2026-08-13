// Double arithmetic: 3.5 + 3.5 = 7.0 → int 7.
// expect: 7
package main;
int main() {
    double a = 3.5;
    double b = 3.5;
    return (int)(a + b);
}
