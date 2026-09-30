#ifndef FAKECC_COMMON_H
#define FAKECC_COMMON_H

#include <stddef.h>

/* Maximum number of parameters a single function may declare.  The SysV AMD64
 * ABI passes the first 6 (integer) / 8 (SSE) args in registers and the rest on
 * the stack, so there is no hard hardware limit — this is a compiler cap.
 * Raised from 16 to 1024 to compile GCC torture tests like 900313-1.c (32 args).
 * Call-argument storage on IR_CALL is heap-allocated (see IR_CALL_MAX_ARGS);
 * do not embed [MAX_PARAMS]-sized arrays in hot IR/codegen structs. */
#define MAX_PARAMS 1024

/* Error codes returned by die_at and compiler pipeline entry points. */
enum {
    FAKECC_OK = 0,
    FAKECC_ERR = 1
};

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

/* Error reporting — records the error, prints to stderr, and returns
 * FAKECC_ERR.  Does not abort the process: callers must propagate the
 * error upward.  The fakecc CLI checks at the top level and exits. */
int die_at(const char *file, int line, int col, const char *fmt, ...);

/* Last-error state for library callers and pipeline layers. */
void fakecc_clear_error(void);
int fakecc_had_error(void);
int fakecc_error_code(void);
const char *fakecc_error_message(void);
SourceLoc fakecc_error_loc(void);

#endif /* FAKECC_COMMON_H */
