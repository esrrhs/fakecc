// expect: 3
// Array of function pointers: fs[1](5, 2) = sub(5, 2) = 3.
package main;
int add(int a, int b) { return a + b; }
int sub(int a, int b) { return a - b; }
int main() {
    int (*fs[2])(int, int);
    fs[0] = add;
    fs[1] = sub;
    return fs[1](5, 2);
}
