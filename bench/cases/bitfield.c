/* Bitfield read and write, including a packed struct. */
package main;
import runtime;

struct Flags {
    unsigned a : 5;
    unsigned b : 11;
    unsigned c : 7;
    unsigned d : 9;
};

struct __attribute__((packed)) Pack {
    unsigned a : 7;
    unsigned b : 10;
    unsigned c : 15;
};

enum { N = 12000 };
enum { ROUNDS = 320 };

static struct Flags flags[N];
static struct Pack packs[N];

int main(void) {
    int i, r;
    unsigned sum;

    for (i = 0; i < N; i++) {
        flags[i].a = (unsigned)(i & 31);
        flags[i].b = (unsigned)(i & 2047);
        flags[i].c = (unsigned)(i & 127);
        flags[i].d = (unsigned)(i & 511);
        packs[i].a = (unsigned)(i & 127);
        packs[i].b = (unsigned)(i & 1023);
        packs[i].c = (unsigned)(i & 32767);
    }
    for (r = 0; r < ROUNDS; r++) {
        for (i = 0; i < N; i++) {
            flags[i].a = flags[i].a + 1u;
            flags[i].b = flags[i].b ^ flags[i].c;
            flags[i].d = flags[i].d + flags[i].a;
            packs[i].a = packs[i].a + packs[i].b;
            packs[i].c = packs[i].c ^ (unsigned)r;
        }
    }
    sum = 0;
    for (i = 0; i < N; i++)
        sum += flags[i].a + flags[i].b + flags[i].c + flags[i].d
             + packs[i].a + packs[i].b + packs[i].c;
    runtime.printf("bitfield %u\n", sum);
    return 0;
}
