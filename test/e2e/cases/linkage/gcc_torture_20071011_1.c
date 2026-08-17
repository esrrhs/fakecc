// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20071011-1.c
// Tests global array initialization and access
package main;

int arr[5] = {10, 20, 30, 40, 50};

int main() {
    int sum = 0;
    for (int i = 0; i < 5; i++)
        sum += arr[i];
    if (sum != 150) return 1;
    return 0;
}
