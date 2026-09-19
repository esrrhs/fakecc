// expect: 0
// __builtin_offsetof supports member[index].field designators.
package main;
struct Inner { int x; int y; };
struct S { struct Inner a[4]; int z; };
int main(void) {
    int off = (int)__builtin_offsetof(struct S, a[2].y);
    if (off != (int)(2 * sizeof(struct Inner) + sizeof(int))) return 1;
    if ((int)__builtin_offsetof(struct S, a[0].x) != 0) return 2;
    return 0;
}
