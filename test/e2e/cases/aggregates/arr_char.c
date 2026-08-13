// expect: 5
package main;
int main() {
    char a[5];
    a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4; a[4] = 5;
    int s = a[0] + a[1] + a[2] - a[3] - a[4] + 8;
    return s;   // 1+2+3-4-5+8 = 5
}
