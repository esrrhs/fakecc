// expect: 42
// Basic typedef: alias int as MyInt and declare a variable with it.
package main;
typedef int MyInt;
int main() {
    MyInt x = 42;
    return x;
}
