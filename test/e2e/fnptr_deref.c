// expect: 7
// Explicit dereference of function pointer: (*fp)(3, 4).
package main;
int add(int a, int b) { return a + b; }
int main() {
    int (*fp)(int, int) = &add;
    return (*fp)(3, 4);
}
