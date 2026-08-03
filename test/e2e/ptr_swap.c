// expect: 12
// Classic swap through pointers.
package main;
int swap(int *a, int *b) {
    int t = *a;
    *a = *b;
    *b = t;
    return 0;
}
int main() {
    int x = 1;
    int y = 2;
    swap(&x, &y);
    return x + y * 10;   // 2 + 10 = 12
}
