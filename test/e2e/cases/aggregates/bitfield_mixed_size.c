// expect: 0
// Mixed declared types pack by bit position (GCC/SysV).  A following non-bitfield
// can occupy leftover bytes of the unit.  Packed structs concatenate bits with
// no padding.
package main;
struct Mix { unsigned char a : 4; unsigned int b : 4; };
struct MixChar { unsigned char a : 4; unsigned int b : 4; char c; };
struct __attribute__((packed)) Pack { unsigned char a : 7; unsigned int b : 10; };
int main(void) {
    if (sizeof(struct Mix) != 4) return 1;
    struct Mix m;
    m.a = 1;
    m.b = 2;
    if (m.a != 1) return 2;
    if (m.b != 2) return 3;
    if (sizeof(struct MixChar) != 4) return 4;
    struct MixChar s;
    s.a = 1;
    s.b = 2;
    s.c = 0x55;
    if (s.a != 1) return 5;
    if (s.b != 2) return 6;
    if (s.c != 0x55) return 7;
    if (sizeof(struct Pack) != 3) return 8;
    struct Pack p;
    p.a = 3;
    p.b = 0x1A5;
    if (p.a != 3) return 9;
    if (p.b != 0x1A5) return 10;
    return 0;
}
