/* Builtin freestanding string/memory routines — FakeCC dialect, no libc. */
package runtime;

/* Word copies. x86-64 and arm64 both allow unaligned accesses, and a
 * self-hosted compile spends most of its time in these loops (malloc
 * zeroes every block, regalloc copies bitsets). A byte loop there is
 * several times slower than the libc the gcc-built compiler links. */

static void copy_words(unsigned char *d, const unsigned char *s, size_t n) {
    while (n >= 32) {
        unsigned long long w0 = *(unsigned long long *)s;
        unsigned long long w1 = *(unsigned long long *)(s + 8);
        unsigned long long w2 = *(unsigned long long *)(s + 16);
        unsigned long long w3 = *(unsigned long long *)(s + 24);
        *(unsigned long long *)d = w0;
        *(unsigned long long *)(d + 8) = w1;
        *(unsigned long long *)(d + 16) = w2;
        *(unsigned long long *)(d + 24) = w3;
        d = d + 32;
        s = s + 32;
        n = n - 32;
    }
    while (n >= 8) {
        *(unsigned long long *)d = *(unsigned long long *)s;
        d = d + 8;
        s = s + 8;
        n = n - 8;
    }
    while (n > 0) {
        *d = *s;
        d = d + 1;
        s = s + 1;
        n = n - 1;
    }
}

void *memcpy(void *dst, const void *src, size_t n) {
    copy_words((unsigned char *)dst, (const unsigned char *)src, n);
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    if (d == s || n == 0) return dst;
    if ((size_t)d < (size_t)s) {
        copy_words(d, s, n);
        return dst;
    }
    d = d + n;
    s = s + n;
    while (n >= 8) {
        d = d - 8;
        s = s - 8;
        n = n - 8;
        *(unsigned long long *)d = *(unsigned long long *)s;
    }
    while (n > 0) {
        d = d - 1;
        s = s - 1;
        n = n - 1;
        *d = *s;
    }
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    unsigned char b = (unsigned char)c;
    unsigned long long w = (unsigned long long)b;
    w = w | (w << 8);
    w = w | (w << 16);
    w = w | (w << 32);
    while (n >= 32) {
        *(unsigned long long *)d = w;
        *(unsigned long long *)(d + 8) = w;
        *(unsigned long long *)(d + 16) = w;
        *(unsigned long long *)(d + 24) = w;
        d = d + 32;
        n = n - 32;
    }
    while (n >= 8) {
        *(unsigned long long *)d = w;
        d = d + 8;
        n = n - 8;
    }
    while (n > 0) {
        *d = b;
        d = d + 1;
        n = n - 1;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    while (n >= 8) {
        unsigned long long wx = *(const unsigned long long *)x;
        unsigned long long wy = *(const unsigned long long *)y;
        if (wx != wy) break;
        x = x + 8;
        y = y + 8;
        n = n - 8;
    }
    while (n > 0) {
        if (*x != *y) return (int)*x - (int)*y;
        x = x + 1;
        y = y + 1;
        n = n - 1;
    }
    return 0;
}

/* True when an already-loaded aligned word contains a zero byte. */
static int word_has_zero(unsigned long long w) {
    unsigned long long m = w - 0x0101010101010101ULL;
    return (m & ~w & 0x8080808080808080ULL) != 0;
}

size_t strlen(const char *s) {
    const char *p = s;
    while (((size_t)p & 7) != 0) {
        if (*p == 0) return (size_t)(p - s);
        p = p + 1;
    }
    while (!word_has_zero(*(const unsigned long long *)p)) p = p + 8;
    while (*p != 0) p = p + 1;
    return (size_t)(p - s);
}

int strcmp(const char *a, const char *b) {
    if (((size_t)a & 7) == 0 && ((size_t)b & 7) == 0) {
        while (1) {
            unsigned long long wa = *(const unsigned long long *)a;
            unsigned long long wb = *(const unsigned long long *)b;
            if (wa != wb || word_has_zero(wa)) break;
            a = a + 8;
            b = b + 8;
        }
    }
    while (*a != 0 && *a == *b) {
        a = a + 1;
        b = b + 1;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

int strncmp(const char *a, const char *b, size_t n) {
    size_t i = 0;
    while (i < n) {
        if (a[i] != b[i])
            return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
        if (a[i] == 0) return 0;
        i = i + 1;
    }
    return 0;
}

char *strcpy(char *dst, const char *src) {
    size_t i = 0;
    while (1) {
        dst[i] = src[i];
        if (src[i] == 0) break;
        i = i + 1;
    }
    return dst;
}

/* ISO C strcat: copy src onto the terminating NUL of dst and return dst.
 * Same contract as glibc/GCC: dest and src must not overlap. */
char *strcat(char *dst, const char *src) {
    size_t i = 0;
    while (dst[i] != 0) i = i + 1;
    size_t j = 0;
    while (1) {
        dst[i + j] = src[j];
        if (src[j] == 0) break;
        j = j + 1;
    }
    return dst;
}

/* ISO C strncat: append at most n chars of src onto dst, always NUL-terminate.
 * Same contract as glibc/GCC: dest and src must not overlap. */
char *strncat(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (dst[i] != 0) i = i + 1;
    size_t j = 0;
    while (j < n && src[j] != 0) {
        dst[i + j] = src[j];
        j = j + 1;
    }
    dst[i + j] = 0;
    return dst;
}

char *strncpy(char *dst, const char *src, size_t n) {
    size_t i = 0;
    while (i < n && src[i] != 0) {
        dst[i] = src[i];
        i = i + 1;
    }
    while (i < n) {
        dst[i] = 0;
        i = i + 1;
    }
    return dst;
}

/* ISO C strspn: length of the initial segment of s that consists only of
 * characters from accept. Same contract as glibc/GCC. */
size_t strspn(const char *s, const char *accept) {
    size_t i = 0;
    while (s[i] != 0) {
        const char *p = accept;
        int found = 0;
        while (*p != 0) {
            if (s[i] == *p) {
                found = 1;
                break;
            }
            p = p + 1;
        }
        if (found == 0) break;
        i = i + 1;
    }
    return i;
}

char *strchr(const char *s, int c) {
    char ch = (char)c;
    size_t i = 0;
    while (1) {
        if (s[i] == ch) return (char *)(s + i);
        if (s[i] == 0) return 0;
        i = i + 1;
    }
}

char *strrchr(const char *s, int c) {
    char ch = (char)c;
    char *last = 0;
    size_t i = 0;
    while (1) {
        if (s[i] == ch) last = (char *)(s + i);
        if (s[i] == 0) return last;
        i = i + 1;
    }
}

char *strstr(const char *hay, const char *needle) {
    if (needle[0] == 0) return (char *)hay;
    size_t i = 0;
    while (hay[i] != 0) {
        size_t j = 0;
        while (needle[j] != 0 && hay[i + j] == needle[j]) j = j + 1;
        if (needle[j] == 0) return (char *)(hay + i);
        i = i + 1;
    }
    return 0;
}

char *strdup(const char *s) {
    size_t n = strlen(s);
    char *p = (char *)malloc(n + 1);
    if (p == 0) return 0;
    memcpy(p, s, n + 1);
    return p;
}

char *strerror(int n) {
    (void)n;
    return "error";
}

void *memchr(const void *s, int c, unsigned long n) {
    const unsigned char *p = (const unsigned char *)s;
    unsigned char ch = (unsigned char)c;
    unsigned long i = 0;
    if (s == 0) return 0;
    while (i < n) {
        if (p[i] == ch) return (void *)(p + i);
        i = i + 1;
    }
    return 0;
}
