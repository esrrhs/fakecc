// expect: 0
// Comma extra declarators after a function declaration (K&R / GNU).
package main;
int foo(int), bar(int);
int *p0(int), *p1(int);
int foo(int x) { return x + 1; }
int bar(int x) { return x + 2; }
int *p0(int x) { (void)x; return 0; }
int *p1(int x) { (void)x; return 0; }
int main(void) {
    if (foo(1) + bar(1) != 5) return 1;
    if (p0(0) || p1(0)) return 2;
    return 0;
}
