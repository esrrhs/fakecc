// expect: 7
// Typedef for a function pointer type, then use it.
package main;
int add(int a, int b) { return a + b; }
typedef int (*BinOp)(int, int);
int main() {
    BinOp fp = add;
    return fp(3, 4);
}
