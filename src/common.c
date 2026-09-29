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
    if (n == 0) return;
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
    /* Zero-fill: stage1's mmap allocator returns recycled dirty chunks, and
     * any unread field then makes register allocation (and bootstrap) depend
     * on ASLR.  gcc-built fakecc must see the same zeros. */
    if (n == 0) n = 1;
    void *p = calloc(1, n);
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

static int g_err_code = FAKECC_OK;
static SourceLoc g_err_loc;
static char g_err_msg[2048];

void fakecc_clear_error(void) {
    g_err_code = FAKECC_OK;
    g_err_loc.file = NULL;
    g_err_loc.line = 0;
    g_err_loc.col = 0;
    g_err_msg[0] = '\0';
}

int fakecc_had_error(void) {
    return g_err_code != FAKECC_OK;
}

int fakecc_error_code(void) {
    return g_err_code;
}

const char *fakecc_error_message(void) {
    return g_err_msg;
}

SourceLoc fakecc_error_loc(void) {
    return g_err_loc;
}

int die_at(const char *file, int line, int col, const char *fmt, ...) {
    /* Already recorded a fatal error — keep the first message and let
     * callers keep propagating FAKECC_ERR without flooding stderr. */
    if (g_err_code != FAKECC_OK) {
        return FAKECC_ERR;
    }

    va_list ap;
    va_start(ap, fmt);

    g_err_loc.file = file;
    g_err_loc.line = line;
    g_err_loc.col = col;
    g_err_code = FAKECC_ERR;

    /* Format the message body (without the location prefix). */
    {
        va_list ap_msg;
        va_copy(ap_msg, ap);
        vsnprintf(g_err_msg, sizeof(g_err_msg), fmt, ap_msg);
        va_end(ap_msg);
    }

    fprintf(stderr, "%s:%d:%d: error: ", file ? file : "(unknown)", line, col);
    vfprintf(stderr, fmt, ap);
    fprintf(stderr, "\n");
    va_end(ap);
    return FAKECC_ERR;
}
