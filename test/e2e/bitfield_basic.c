// expect: 0
// Adjacent unsigned bitfields pack into one storage unit and round-trip.
package main;
struct F {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 4;
};
int main(void) {
    struct F f;
    f.a = 5;
    f.b = 17;
    f.c = 9;
    if (f.a != 5) return 1;
    if (f.b != 17) return 2;
    if (f.c != 9) return 3;
    if (f.a + f.b + f.c != 31) return 4;
    return 0;
}
