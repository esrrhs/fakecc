// expect: 99
package main;
int main() {
    int x = 0;
    int *p = &x;
    *p = 99;
    return x;
}
