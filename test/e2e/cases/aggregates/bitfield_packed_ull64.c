// expect: 0
// Packed 1-bit + 64-bit field spans 9 bytes; stores must not drop the high bit.
package main;
struct __attribute__((packed)) P {
    unsigned char a : 1;
    unsigned long long b : 64;
};
int main(void) {
    struct P p;
    p.a = 1;
    p.b = 0x8000000000000001ULL;
    if (p.a != 1) return 1;
    if (p.b != 0x8000000000000001ULL) return 2;
    p.b = p.b + 1;
    if (p.b != 0x8000000000000002ULL) return 3;
    if (p.a != 1) return 4;
    p.b++;
    if (p.b != 0x8000000000000003ULL) return 5;
    if (p.a != 1) return 6;
    p.b += 1;
    if (p.b != 0x8000000000000004ULL) return 7;
    struct P q = { 1, 0x8000000000000001ULL };
    if (q.a != 1) return 8;
    if (q.b != 0x8000000000000001ULL) return 9;
    if (sizeof(struct P) != 9) return 10;
    return 0;
}
