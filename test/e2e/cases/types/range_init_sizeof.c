// expect: 0
package main;

int main(void) {
    int a[4] = { [0 ... 3] = 7 };
    if (a[0] != 7 || a[3] != 7) return 1;
    int n = 3;
    if (sizeof(+n) != sizeof(int)) return 2;
    if (sizeof(n + 1) != sizeof(int)) return 3;
    if (sizeof(n ? n : n) != sizeof(int)) return 4;
    return 0;
}
