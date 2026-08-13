// expect: 99
package main;
int main() {
    int x = 42;
    int r = 0;
    switch (x) {
        case 0: r = 10; break;
        case 1: r = 15; break;
        default: r = 99; break;
    }
    return r;
}
