// expect: 55
// Fill 1..10 into array, then sum.
package main;
int main() {
    int a[10];
    int i = 0;
    while (i < 10) {
        a[i] = i + 1;
        i = i + 1;
    }
    int sum = 0;
    i = 0;
    while (i < 10) {
        sum = sum + a[i];
        i = i + 1;
    }
    return sum;
}
