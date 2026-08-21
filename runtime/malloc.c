/* mmap-backed freelist allocator — FakeCC dialect. */
package runtime;

struct chunk {
    size_t size;
    struct chunk *next_free;
};

static struct chunk *freelist_head;

static size_t align8(size_t n) {
    if (n > ((size_t)-8)) return (size_t)-8;
    return (n + 7) & ~((size_t)7);
}

static void *map_anon(size_t n) {
    long p = __syscall(9, 0, (long)n, 3, 0x22, -1, 0);
    if (p < 0) return 0;
    return (void *)p;
}

static int chunks_adjacent(struct chunk *a, struct chunk *b) {
    return (char *)a + sizeof(struct chunk) + a->size == (char *)b;
}

static void heap_grow(size_t need) {
    size_t extra = sizeof(struct chunk) + 64;
    if (need > ((size_t)-1) - extra) return;
    size_t region = need + extra;
    region = (region + 4095) & ~((size_t)4095);
    if (region < 1048576) region = 1048576;
    char *p = (char *)map_anon(region);
    if (p == 0) return;
    struct chunk *c = (struct chunk *)p;
    c->size = region - sizeof(struct chunk);
    c->next_free = 0;
    /* Insert by address so later frees can coalesce with this region. */
    struct chunk **prev = &freelist_head;
    struct chunk *n = freelist_head;
    while (n && n < c) {
        prev = &n->next_free;
        n = n->next_free;
    }
    c->next_free = n;
    *prev = c;
    if (n && chunks_adjacent(c, n)) {
        c->size = c->size + sizeof(struct chunk) + n->size;
        c->next_free = n->next_free;
    }
}

static int already_free(struct chunk *c) {
    struct chunk *p = freelist_head;
    while (p) {
        if (p == c) return 1;
        p = p->next_free;
    }
    return 0;
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
    c->next_free = 0;
    return (char *)c + sizeof(struct chunk);
}

void free(void *p) {
    if (p == 0) return;
    struct chunk *c = (struct chunk *)((char *)p - sizeof(struct chunk));
    if (already_free(c)) return;
    struct chunk *left = 0;
    struct chunk *n = freelist_head;
    while (n && n < c) {
        left = n;
        n = n->next_free;
    }
    c->next_free = n;
    if (left) left->next_free = c;
    else freelist_head = c;
    if (n && chunks_adjacent(c, n)) {
        c->size = c->size + sizeof(struct chunk) + n->size;
        c->next_free = n->next_free;
    }
    if (left && chunks_adjacent(left, c)) {
        left->size = left->size + sizeof(struct chunk) + c->size;
        left->next_free = c->next_free;
    }
}

void *calloc(size_t n, size_t m) {
    if (m != 0 && n > ((size_t)-1) / m) return 0;
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
