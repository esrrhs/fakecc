// expect: 42
package main;
int main() {
    int x = 42;
    int *p = &x;
    return *p;
}
