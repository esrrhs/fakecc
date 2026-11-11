// expect: 0
// _Alignof / sizeof stored, reused after a clobbering loop (align O1).
package main;

struct __attribute__((aligned(16))) A16 { int x; };
struct __attribute__((packed)) Pack { char tag; int val; };

static int buf[64];

int main(void) {
    int i;
    unsigned a, p, s, acc;
    a = (unsigned)_Alignof(struct A16);
    p = (unsigned)_Alignof(struct Pack);
    s = (unsigned)sizeof(struct Pack);
    for (i = 0; i < 64; i++)
        buf[i] = i;
    acc = a + p + s + (unsigned)buf[3];
    return (acc == 16u + 1u + 5u + 3u && a == 16u && p == 1u && s == 5u) ? 0 : 1;
}
