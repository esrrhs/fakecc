// expect: 0
package main;

__thread int a;
__thread long b;
__thread char buf[8];
__thread short c;

int main() {
    a = 123;
    b = 456789;
    c = 99;
    buf[0] = 10;
    buf[1] = 20;

    if (a != 123) return 1;
    if (b != 456789) return 2;
    if (c != 99) return 3;
    if (buf[0] != 10 || buf[1] != 20) return 4;
    return 0;
}
