// expect: 11
// 2D array indexing: a[i][j] = i*3 + j, then read a[2][2] + a[1][0] = 8+3 = 11.
package main;
int main() {
    int a[3][3];
    int i = 0;
    while (i < 3) {
        int j = 0;
        while (j < 3) {
            a[i][j] = i * 3 + j;
            j = j + 1;
        }
        i = i + 1;
    }
    return a[2][2] + a[1][0];
}
