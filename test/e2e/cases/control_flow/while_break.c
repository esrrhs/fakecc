// expect: 55
// break inside a while loop.
package main;
int main() {
    int s = 0;
    int i = 0;
    while (i < 1000) {
        i = i + 1;
        if (i > 10) { break; }
        s = s + i;
    }
    return s;
}
