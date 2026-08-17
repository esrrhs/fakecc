// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/pr32929.c
// Tests that post-increment of pointer returns old value
package main;

int main() {
    int arr[3] = {10, 20, 30};
    int *p = arr;
    int first = *p++;
    int second = *p++;
    int third = *p;
    if (first != 10) return 1;
    if (second != 20) return 2;
    if (third != 30) return 3;
    return 0;
}
