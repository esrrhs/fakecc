// expect: 10
// static global variable — readable and writable from a function.
package main;
static int g = 10;
int main() {
    return g;
}
