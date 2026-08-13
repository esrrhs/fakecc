// expect: 0
// Writing zero clears only the targeted field bits.
package main;
struct F {
    unsigned a : 5;
    unsigned b : 5;
    unsigned c : 5;
};
int main(void) {
    struct F f;
    f.a = 31;
    f.b = 16;
    f.c = 7;
    f.b = 0;
    if (f.a != 31) return 1;
    if (f.b != 0) return 2;
    if (f.c != 7) return 3;
    return 0;
}
