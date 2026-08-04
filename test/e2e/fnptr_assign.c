// expect: 7
// Function decays to pointer on assignment — no `&` needed.
package main;
int add(int a, int b) { return a + b; }
int main() {
    int (*fp)(int, int) = add;
    return fp(3, 4);
}
