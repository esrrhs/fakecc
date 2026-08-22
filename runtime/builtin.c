/* Shared typedefs for the runtime package — parsed first (name sorts first). */
package runtime;

typedef unsigned long size_t;
typedef long ssize_t;

/* glibc errno; strto* sets this to 34 (ERANGE) on overflow. */
int errno;

struct FILE {
    int fd;
    int writable;
    int eof;
    int err;
    int buf_len;
    int buf_cap;
    int has_ungot;
    int ungot;
    char buf[1024];
};
typedef struct FILE FILE;
