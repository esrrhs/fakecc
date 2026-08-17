// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20020227-1.c
package main;

int main() {
    long long a = 0x123456789ABCDEF0LL;
    long long b = 0x0F0F0F0F0F0F0F0FLL;
    long long c = a & b;
    if (c != 0x020406080A0C0E00LL) return 1;
    return 0;
}
