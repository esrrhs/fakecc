// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20091229-1.c
package main;

long long foo(long long v) { return v / -0x080000000LL; }
int main(int argc, char **argv) { if (foo(0x080000000LL) != -1) return 1; return 0; }