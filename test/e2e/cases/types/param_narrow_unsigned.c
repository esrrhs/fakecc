// expect: 20
// A wide literal (> INT_MAX) passed to a narrow unsigned parameter must be
// converted to the parameter type at the call site, not sign-extended to 64
// bits. Regression for the "implicit argument conversion" known defect.
package main;
int f(unsigned int u) { return (int)(u / 200000000); }
int main(void) { return f(4000000000); }
