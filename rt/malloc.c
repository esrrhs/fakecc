/* mmap-backed freelist allocator — FakeCC dialect. */
package main;

typedef unsigned long size_t;

struct chunk {
    size_t size;
    int free;
    struct chunk *next;
};

static struct chunk *heap_head;

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
    if (region < 65536) region = 65536;
    char *p = (char *)map_anon(region);
    if (p == 0) return;
    struct chunk *c = (struct chunk *)p;
    c->size = region - sizeof(struct chunk);
    c->free = 1;
    c->next = heap_head;
    heap_head = c;
}

extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);

void *malloc(size_t n) {
    if (n == 0) n = 1;
    n = align8(n);
    if (heap_head == 0) heap_grow(n);
    struct chunk *c = heap_head;
    struct chunk *best = 0;
    while (c) {
        if (c->free && c->size >= n) {
            best = c;
            break;
        }
        c = c->next;
    }
    if (best == 0) {
        heap_grow(n);
        c = heap_head;
        while (c) {
            if (c->free && c->size >= n) {
                best = c;
                break;
            }
            c = c->next;
        }
        if (best == 0) return 0;
    }
    if (best->size >= n + sizeof(struct chunk) + 8) {
        char *base = (char *)best;
        struct chunk *rest = (struct chunk *)(base + sizeof(struct chunk) + n);
        rest->size = best->size - n - sizeof(struct chunk);
        rest->free = 1;
        rest->next = best->next;
        best->size = n;
        best->next = rest;
    }
    best->free = 0;
    return (char *)best + sizeof(struct chunk);
}

void free(void *p) {
    if (p == 0) return;
    struct chunk *c = (struct chunk *)((char *)p - sizeof(struct chunk));
    c->free = 1;
    if (c->next && c->next->free) {
        c->size = c->size + sizeof(struct chunk) + c->next->size;
        c->next = c->next->next;
    }
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
