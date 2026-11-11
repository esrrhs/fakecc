// expect: 0
/* Vector ops on a packed (unaligned) field must not use alignment-sensitive
 * moves or packed-integer memory operands.  A raw `v4si *` to an odd
 * address is UB in GCC (it may emit movdqa / paddd mem and #GP); the
 * packed field access is the well-defined unaligned case. */
package main;

typedef int v4si __attribute__((vector_size(16)));

struct __attribute__((packed)) P {
    char pad;
    v4si v;
};

int main(void) {
    struct P s, t;
    s.pad = 9;
    t.pad = 10;
    s.v[0] = 1; s.v[1] = 2; s.v[2] = 3; s.v[3] = 4;
    t.v[0] = 5; t.v[1] = 6; t.v[2] = 7; t.v[3] = 8;
    {
        v4si tmp = s.v + t.v;
        s.v = tmp ^ t.v;
    }
    if (s.pad != 9 || t.pad != 10)
        return 2;
    if (s.v[0] != 3 || s.v[1] != 14 || s.v[2] != 13 || s.v[3] != 4)
        return 3;
    return 0;
}
