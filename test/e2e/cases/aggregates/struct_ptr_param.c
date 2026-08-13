// expect: 142
// Passing a struct pointer to a function.
package main;
struct P { int x; int y; };
int addp(struct P *p) { return p->x + p->y; }
int main() {
    struct P p;
    p.x = 100; p.y = 42;
    return addp(&p);
}
