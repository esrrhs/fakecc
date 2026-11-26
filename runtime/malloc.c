/* mmap-backed freelist allocator — FakeCC dialect.
 *
 * free() used to walk an address-ordered list to coalesce. A self-hosted
 * compile frees millions of blocks and that walk was the whole profile.
 * Chunks carry a footer and a doubly-linked size-class bin, so free finds
 * the physical neighbours and unlinks them in constant time. malloc still
 * returns zeroed memory: a self-hosted compiler must not observe leftover
 * bytes from a recycled block. */
package runtime;

struct chunk {
    size_t size;             /* usable bytes; multiple of 16 */
    size_t user_size;
    struct chunk *next_free;
    struct chunk *prev_free;
    size_t state;            /* 1 on a bin, 0 once handed out */
    size_t pad;              /* header is 48 bytes, so the user pointer is 16-aligned */
};

/* 16, 32, ... and a final bin for everything >= 16MB. */
static struct chunk *bins[21];

static size_t __fakecc_alloc_align16(size_t n) {
    if (n > ((size_t)-16)) return (size_t)-16;
    return (n + 15) & ~((size_t)15);
}

static int __fakecc_alloc_floor_bin(size_t sz) {
    int b = 0;
    size_t s = 16;
    int best = 0;
    while (b < 20 && (s + s) <= sz) {
        s = s + s;
        b = b + 1;
        best = b;
    }
    return best;
}

static void *map_anon(size_t n) {
    long p = __syscall(9, 0, (long)n, 3, __fakecc_map_private_anon, -1, 0);
    if (p < 0) return 0;
    return (void *)p;
}

static size_t *__fakecc_alloc_footer_of(struct chunk *c) {
    return (size_t *)((char *)c + sizeof(struct chunk) + c->size);
}

static void __fakecc_alloc_write_footer(struct chunk *c) {
    *__fakecc_alloc_footer_of(c) = c->size;
}

static struct chunk *__fakecc_alloc_phys_next(struct chunk *c) {
    return (struct chunk *)((char *)__fakecc_alloc_footer_of(c) + 16);
}

static struct chunk *__fakecc_alloc_phys_prev(struct chunk *c) {
    size_t prev_sz = *(size_t *)((char *)c - 16);
    if (prev_sz == 0) return 0;
    return (struct chunk *)((char *)c - 16 - prev_sz - sizeof(struct chunk));
}

static void __fakecc_alloc_bin_unlink(struct chunk *c) {
    int b = __fakecc_alloc_floor_bin(c->size);
    if (c->prev_free) c->prev_free->next_free = c->next_free;
    else bins[b] = c->next_free;
    if (c->next_free) c->next_free->prev_free = c->prev_free;
    c->next_free = 0;
    c->prev_free = 0;
    c->state = 0;
}

static void __fakecc_alloc_bin_push(struct chunk *c) {
    int b = __fakecc_alloc_floor_bin(c->size);
    c->prev_free = 0;
    c->next_free = bins[b];
    if (bins[b]) bins[b]->prev_free = c;
    bins[b] = c;
    c->state = 1;
    __fakecc_alloc_write_footer(c);
}

static void heap_grow(size_t need) {
    /* Overhead in a fresh mapping: 16-byte leading dummy footer + chunk header
     * + 16-byte chunk footer + end sentinel header. */
    size_t extra = 2 * sizeof(struct chunk) + 32;
    size_t region;
    char *p;
    struct chunk *c;
    struct chunk *end;
    if (need > ((size_t)-1) - extra - 4095) return;
    region = need + extra;
    region = (region + 4095) & ~((size_t)4095);
    if (region < 1048576) region = 1048576;
    p = (char *)map_anon(region);
    if (p == 0) return;
    __asan_unpoison_memory_region(p, region);
    /* 16-byte dummy footer so the first chunk has no physical predecessor. */
    *(size_t *)p = 0;
    c = (struct chunk *)(p + 16);
    end = (struct chunk *)(p + region - sizeof(struct chunk));
    c->size = (size_t)((char *)end - ((char *)c + sizeof(struct chunk)) - 16);
    c->user_size = 0;
    c->pad = 0;
    end->size = 0;
    end->user_size = 0;
    end->next_free = 0;
    end->prev_free = 0;
    end->state = 0; /* sentinel: allocated, so free() never coalesces past the mapping */
    end->pad = 0;
    __fakecc_alloc_bin_push(c);
}

static struct chunk *__fakecc_alloc_take_fit(size_t n) {
    int b = __fakecc_alloc_floor_bin(n);
    struct chunk *c = bins[b];
    if (c && c->size >= n) return c;
    if (b == 20) {
        while (c) {
            if (c->size >= n) return c;
            c = c->next_free;
        }
        return 0;
    }
    b = b + 1;
    while (b < 21) {
        if (bins[b]) return bins[b];
        b = b + 1;
    }
    return 0;
}

void *malloc(size_t n) {
    size_t orig_n = n;
    struct chunk *c;
    void *ret;
    if (n == 0) n = 1;
    n = __fakecc_alloc_align16(n);
    c = __fakecc_alloc_take_fit(n);
    if (c == 0) {
        heap_grow(n);
        c = __fakecc_alloc_take_fit(n);
        if (c == 0) return 0;
    }
    __fakecc_alloc_bin_unlink(c);
    if (c->size >= n + sizeof(struct chunk) + 32) {
        size_t old = c->size;
        struct chunk *rest;
        c->size = n;
        rest = (struct chunk *)((char *)c + sizeof(struct chunk) + n + 16);
        rest->size = old - n - sizeof(struct chunk) - 16;
        rest->user_size = 0;
        rest->pad = 0;
        __fakecc_alloc_write_footer(c);
        __fakecc_alloc_bin_push(rest);
    }
    c->state = 0;
    c->user_size = orig_n;
    ret = (char *)c + sizeof(struct chunk);
    __asan_unpoison_memory_region(ret, n);
    memset(ret, 0, n);
    return ret;
}

void free(void *p) {
    struct chunk *c;
    struct chunk *n;
    struct chunk *left;
    if (p == 0) return;
    c = (struct chunk *)((char *)p - sizeof(struct chunk));
    if (c->state == 1) return;
    __asan_poison_memory_region(p, c->size);
    n = __fakecc_alloc_phys_next(c);
    if (n->state == 1) {
        __fakecc_alloc_bin_unlink(n);
        c->size = c->size + 16 + sizeof(struct chunk) + n->size;
    }
    left = __fakecc_alloc_phys_prev(c);
    if (left && left->state == 1) {
        __fakecc_alloc_bin_unlink(left);
        left->size = left->size + 16 + sizeof(struct chunk) + c->size;
        left->user_size = 0;
        __fakecc_alloc_bin_push(left);
        return;
    }
    c->user_size = 0;
    __fakecc_alloc_bin_push(c);
}

void *calloc(size_t n, size_t m) {
    size_t total;
    void *p;
    if (m != 0 && n > ((size_t)-1) / m) return 0;
    total = n * m;
    p = malloc(total);
    if (p == 0) return 0;
    memset(p, 0, total);
    return p;
}

void *realloc(void *p, size_t n) {
    struct chunk *c;
    void *q;
    size_t copy;
    if (p == 0) return malloc(n);
    if (n == 0) {
        free(p);
        return 0;
    }
    c = (struct chunk *)((char *)p - sizeof(struct chunk));
    if (c->size >= n) {
        c->user_size = n;
        __asan_unpoison_memory_region(p, n);
        return p;
    }
    q = malloc(n);
    if (q == 0) return 0;
    copy = c->user_size;
    if (copy == 0 || copy > c->size) copy = c->size;
    if (copy > n) copy = n;
    memcpy(q, p, copy);
    free(p);
    return q;
}
