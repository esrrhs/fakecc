// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20010114-1.c
package main;

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *p = arr;
    int sum = 0;
    for (int i = 0; i < 5; i++) {
        sum += *p++;
    }
    if (sum != 15) return 1;
    return 0;
}
