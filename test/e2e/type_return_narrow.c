// expect: 42
// Function returning a char, called from int context.
package main;
int helper() {
    char c = 42;
    return c;
}
int main() { return helper(); }
