// expect: 15
// Array parameter (int a[]) decays to int*; caller passes an array.
package main;
int sum(int a[], int n) {
    int s = 0;
    int i = 0;
    while (i < n) {
        s = s + a[i];
        i = i + 1;
    }
    return s;
}
int main() {
    int a[5];
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4; a[4] = 5;
    return sum(a, 5);
}
