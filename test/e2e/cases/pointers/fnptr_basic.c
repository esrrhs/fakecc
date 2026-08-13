// expect: 7
// Basic function pointer: assign function to pointer, call indirectly.
package main;
int add(int a, int b) { return a + b; }
int main() {
    int (*fp)(int, int) = &add;
    return fp(3, 4);
}
