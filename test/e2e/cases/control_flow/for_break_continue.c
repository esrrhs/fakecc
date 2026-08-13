// expect: 105
package main;
int main() {
    int s = 0;
    int i;
    for (i = 1; i <= 100; i = i + 1) {
        if (i == 50) { break; }
        if (i > 10) { continue; }
        s = s + i;
    }
    return s + i;
}
