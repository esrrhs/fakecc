// expect: 3
package main;
int main() {
    int x = 10;
    int r = 0;
    if (x == 1) { r = 1; }
    else if (x == 5) { r = 2; }
    else if (x == 10) { r = 3; }
    else { r = 99; }
    return r;
}
