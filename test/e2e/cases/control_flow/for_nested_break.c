// expect: 10
// Nested for; inner break exits inner only.
// i=1: j=1 (2>1 break) → total=1
// i=2: j=1,2 (3>2 break) → total=1+1+2=4
// i=3: j=1,2,3 (4>3 break) → total=4+1+2+3=10
package main;
int main() {
    int total = 0;
    for (int i = 1; i <= 3; i = i + 1) {
        for (int j = 1; j <= 10; j = j + 1) {
            if (j > i) { break; }
            total = total + j;
        }
    }
    return total;
}
