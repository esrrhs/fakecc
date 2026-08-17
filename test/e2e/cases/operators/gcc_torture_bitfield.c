// expect: 0
// Ported from GCC C-Torture suite: gcc.c-torture/execute/920501-1.c
package main;

struct s {
    unsigned short a:1;
    unsigned short b:2;
    unsigned short c:3;
};

int main() {
    struct s x;
    x.a = 1;
    x.b = 2;
    x.c = 3;
    if (x.a != 1 || x.b != 2 || x.c != 3) return 1;
    return 0;
}
