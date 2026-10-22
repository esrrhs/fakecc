// expect: 0
// A narrowing cast must clear (or sign-extend over) the discarded high bits.
// IR_TRUNC used to be a pure register-level no-op on the theory that narrower
// uses would re-mask via the width field — but the comparison opcodes issue a
// 64-bit `cmp` without re-masking, so the leftover half made the compare fail.
//
// This is the shape that broke emit_obj_read during self-hosting: it reads a
// section header's 32-bit sh_type with a helper that fetches 8 bytes and casts
// down, so sh_flags rode along in the high half and `type == SHT_PROGBITS`
// compared false for every section with non-zero flags — .text among them.
package main;
unsigned long rd64(const unsigned char *p) {
    unsigned long v = 0;
    for (int i = 0; i < 8; i++) v = v | ((unsigned long)p[i] << (8 * i));
    return v;
}
unsigned int rd32(const unsigned char *p) { return (unsigned int)rd64(p); }
int main() {
    unsigned char b[8];
    b[0] = 1; b[1] = 0; b[2] = 0; b[3] = 0;   // low half  = 1
    b[4] = 6; b[5] = 0; b[6] = 0; b[7] = 0;   // high half = 6, must be dropped
    unsigned int t = rd32(b);
    if (t != 1) return 1;
    if (!(t == 1)) return 2;
    // Signed narrowing keeps its sign rather than the discarded bits.
    unsigned char n[8];
    n[0] = 0xff; n[1] = 0xff; n[2] = 0xff; n[3] = 0xff;
    n[4] = 6; n[5] = 0; n[6] = 0; n[7] = 0;
    int s = (int)rd64(n);
    if (s != -1) return 3;
    return 0;
}
