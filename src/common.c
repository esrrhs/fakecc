#include "fakecc/common.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Buffer                                                              */
/* ------------------------------------------------------------------ */

void buffer_init(Buffer *b) {
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

void buffer_free(Buffer *b) {
    free(b->data);
    b->data = NULL;
    b->len = 0;
    b->cap = 0;
}

static void buffer_grow(Buffer *b, size_t need) {
    if (b->len + need <= b->cap) return;
    size_t new_cap = b->cap ? b->cap * 2 : 256;
    while (new_cap < b->len + need) new_cap *= 2;
    b->data = realloc(b->data, new_cap);
    if (!b->data) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    b->cap = new_cap;
}

void buffer_append(Buffer *b, const char *s, size_t n) {
    buffer_grow(b, n);
    memcpy(b->data + b->len, s, n);
    b->len += n;
}

void buffer_appendf(Buffer *b, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);

    va_list ap2;
    va_copy(ap2, ap);
    int n = vsnprintf(NULL, 0, fmt, ap2);
    va_end(ap2);

    if (n < 0) {
        fprintf(stderr, "fakecc: vsnprintf failed\n");
        exit(1);
    }

    size_t need = (size_t)n;
    buffer_grow(b, need + 1);
    vsnprintf(b->data + b->len, need + 1, fmt, ap);
    b->len += need;

    va_end(ap);
}

/* ------------------------------------------------------------------ */
/* Checked malloc/realloc                                              */
/* ------------------------------------------------------------------ */

void *xmalloc(size_t n) {
    void *p = malloc(n);
    if (!p) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return p;
}

void *xrealloc(void *p, size_t n) {
    void *q = realloc(p, n);
    if (!q) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    return q;
}

/* ------------------------------------------------------------------ */
/* C99-compatible strdup                                               */
/* ------------------------------------------------------------------ */

char *xstrdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *d = malloc(len);
    if (!d) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    memcpy(d, s, len);
    return d;
}

/* ------------------------------------------------------------------ */
/* Error reporting                                                     */
/* ------------------------------------------------------------------ */

void die_at(const char *file, int line, int col, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    fprintf(stderr, "%s:%d:%d: error: ", file, line, col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    exit(1);
}
