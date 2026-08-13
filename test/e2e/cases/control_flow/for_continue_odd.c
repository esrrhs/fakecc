// expect: 25
// Skip evens using continue.
package main;
int main() {
    int s = 0;
    for (int i = 1; i <= 9; i = i + 1) {
        if (i % 2 == 0) { continue; }
        s = s + i;
    }
    return s;   // 1+3+5+7+9 = 25
}
