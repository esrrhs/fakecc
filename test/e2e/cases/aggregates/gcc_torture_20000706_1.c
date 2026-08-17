// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20000706-1.c
package main;

struct data {
    int x;
    int y;
    int z;
};

struct data get_data(int a, int b, int c) {
    struct data d;
    d.x = a;
    d.y = b;
    d.z = c;
    return d;
}

int main() {
    struct data d = get_data(10, 20, 30);
    if (d.x != 10 || d.y != 20 || d.z != 30) return 1;
    return 0;
}
