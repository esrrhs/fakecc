#ifndef FAKECC_COMMON_H
#define FAKECC_COMMON_H

#include <stddef.h>

/* Source location — shared by tokens, AST, IR, and error reporting */
typedef struct {
    const char *file;    /* pointer to long-lived filename string */
    int line;
    int col;
} SourceLoc;

/* Dynamic byte buffer (for source input, assembly output) */
typedef struct {
    char *data;
    size_t len;
    size_t cap;
} Buffer;

void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_append(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);

/* C99-compatible strdup */
char *xstrdup(const char *s);

/* Checked malloc/realloc — exits on OOM */
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);

/* Error reporting — prints to stderr and exits with code 1 */
void die_at(const char *file, int line, int col, const char *fmt, ...);

#endif /* FAKECC_COMMON_H */
