// expect: 0
// A block-scope unprototyped `extern double g();` still default-promotes
// float arguments to double, matching a file-scope `g()`.
package main;
double g(double x) { return x; }
int main(void) {
    extern double g();
    if (g(1.0f) != 1.0) return 1;
    return 0;
}
