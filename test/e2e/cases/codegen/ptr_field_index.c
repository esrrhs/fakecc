// expect: 0
// `h->d[i]` / `h->d[i-1]` / `h->d[i+1]` on a pointer to a struct field
// array must stay correct when the field-offset ADD is not consecutive
// with the scaled index (flex-style FAM / trailing-array access).
package main;

struct Hdr {
    int n;
    unsigned d[32];
};

static unsigned walk(struct Hdr *h, int n) {
    int i;
    unsigned acc;
    acc = 0;
    for (i = 0; i < n; i = i + 1)
        h->d[i] = h->d[i % 8] + (unsigned)i;
    for (i = 1; i < n - 1; i = i + 1)
        h->d[i] = (h->d[i - 1] * 3u + h->d[i] + h->d[i + 1]) & 0xffffu;
    for (i = 0; i < n; i = i + 2)
        acc = acc + h->d[i];
    acc = acc ^ (unsigned)h->n;
    return acc;
}

int main(void) {
    struct Hdr h;
    int i;
    h.n = 16;
    for (i = 0; i < 16; i = i + 1)
        h.d[i] = (unsigned)(i * 17 + 3);
    if (walk(&h, 16) != 146112) return 1;
    return 0;
}
