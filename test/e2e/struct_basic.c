// expect: 25
package main;
struct Point { int x; int y; };
int main() {
    struct Point p;
    p.x = 3;
    p.y = 4;
    return p.x * p.x + p.y * p.y;
}
