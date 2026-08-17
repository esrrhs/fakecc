// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/20050119-1.c
// Tests array of structs
package main;

struct point {
    int x;
    int y;
};

int main() {
    struct point pts[3];
    pts[0].x = 1; pts[0].y = 2;
    pts[1].x = 3; pts[1].y = 4;
    pts[2].x = 5; pts[2].y = 6;

    int sumx = pts[0].x + pts[1].x + pts[2].x;
    int sumy = pts[0].y + pts[1].y + pts[2].y;
    if (sumx != 9) return 1;
    if (sumy != 12) return 2;
    return 0;
}
