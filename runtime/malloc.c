/* mmap-backed freelist allocator — FakeCC dialect. */
package runtime;

struct chunk {
    size_t size;
    struct chunk *next_free;
};

static struct chunk *freelist_head;

static size_t align8(size_t n) {
    return (n + 7) & ~((size_t)7);
}

static void *map_anon(size_t n) {
    long p = __syscall(9, 0, (long)n, 3, 0x22, -1, 0);
    if (p < 0) return 0;
    return (void *)p;
}

static void heap_grow(size_t need) {
    size_t region = need + sizeof(struct chunk) + 64;
    region = (region + 4095) & ~((size_t)4095);
    if (region < 1048576) region = 1048576;
    char *p = (char *)map_anon(region);
    if (p == 0) return;
    struct chunk *c = (struct chunk *)p;
    c->size = region - sizeof(struct chunk);
    c->next_free = freelist_head;
    freelist_head = c;
}

void *malloc(size_t n) {
    if (n == 0) n = 1;
    n = align8(n);
    if (freelist_head == 0) heap_grow(n);
    struct chunk **prev = &freelist_head;
    struct chunk *c = freelist_head;
    while (c) {
        if (c->size >= n) break;
        prev = &c->next_free;
        c = c->next_free;
    }
    if (c == 0) {
        heap_grow(n);
        prev = &freelist_head;
        c = freelist_head;
        while (c) {
            if (c->size >= n) break;
            prev = &c->next_free;
            c = c->next_free;
        }
        if (c == 0) return 0;
    }
    if (c->size >= n + sizeof(struct chunk) + 8) {
        char *base = (char *)c;
        struct chunk *rest = (struct chunk *)(base + sizeof(struct chunk) + n);
        rest->size = c->size - n - sizeof(struct chunk);
        rest->next_free = c->next_free;
        *prev = rest;
        c->size = n;
    } else {
        *prev = c->next_free;
    }
    return (char *)c + sizeof(struct chunk);
}

void free(void *p) {
    if (p == 0) return;
    struct chunk *c = (struct chunk *)((char *)p - sizeof(struct chunk));
    c->next_free = freelist_head;
    freelist_head = c;
}

void *calloc(size_t n, size_t m) {
    size_t total = n * m;
    void *p = malloc(total);
    if (p == 0) return 0;
    memset(p, 0, total);
    return p;
}

void *realloc(void *p, size_t n) {
    if (p == 0) return malloc(n);
    if (n == 0) {
        free(p);
        return 0;
    }
    struct chunk *c = (struct chunk *)((char *)p - sizeof(struct chunk));
    if (c->size >= n) return p;
    void *q = malloc(n);
    if (q == 0) return 0;
    size_t copy = c->size;
    if (copy > n) copy = n;
    memcpy(q, p, copy);
    free(p);
    return q;
}
