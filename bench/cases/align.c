/* Overaligned structs, packed layout, and _Alignof in a memory-heavy loop. */
package main;
import runtime;

enum { N = 8192 };
enum { ROUNDS = 800 };

struct __attribute__((aligned(16))) A16 {
    unsigned a, b, c, d;
};

struct __attribute__((packed)) Pack {
    unsigned char tag;
    unsigned val;
    unsigned short aux;
};

static struct A16 rows[N];
static struct Pack packs[N];

int main(void) {
    int i, r;
    unsigned acc;
    unsigned align_a16;
    unsigned align_pack;
    unsigned size_pack;

    align_a16 = (unsigned)_Alignof(struct A16);
    align_pack = (unsigned)_Alignof(struct Pack);
    size_pack = (unsigned)sizeof(struct Pack);

    for (i = 0; i < N; i++) {
        rows[i].a = (unsigned)i;
        rows[i].b = (unsigned)(i * 3);
        rows[i].c = (unsigned)(i * 5);
        rows[i].d = (unsigned)(i * 7);
        packs[i].tag = (unsigned char)(i & 255);
        packs[i].val = (unsigned)(i * 13);
        packs[i].aux = (unsigned short)(i * 17);
    }
    acc = align_a16 + align_pack + size_pack;
    for (r = 0; r < ROUNDS; r++) {
        for (i = 0; i < N; i++) {
            rows[i].a = rows[i].a + rows[i].d + (unsigned)r;
            rows[i].b ^= packs[i].val;
            packs[i].val = packs[i].val + rows[i].c + (unsigned)packs[i].tag;
            packs[i].aux = (unsigned short)(packs[i].aux + (unsigned short)rows[i].b);
            packs[i].tag = (unsigned char)(packs[i].tag + (unsigned char)(i & 7));
        }
    }
    for (i = 0; i < N; i += 16) {
        acc += rows[i].a ^ rows[i].c;
        acc += packs[i].val + (unsigned)packs[i].aux + (unsigned)packs[i].tag;
    }
    runtime.printf("align %u %u %u %u\n", acc, align_a16, align_pack, size_pack);
    return 0;
}
