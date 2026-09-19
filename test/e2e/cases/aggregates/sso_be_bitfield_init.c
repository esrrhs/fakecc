// expect: 0
// Big-endian bitfields must pack into a global initializer blob.
package main;
struct B {
    unsigned a : 4;
    unsigned b : 4;
} __attribute__((scalar_storage_order("big-endian")));
struct B g = { 1, 2 };
int main(void) {
    if (g.a != 1) return 1;
    if (g.b != 2) return 2;
    return 0;
}
