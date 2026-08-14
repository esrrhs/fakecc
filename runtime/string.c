/* Builtin freestanding string/memory routines — FakeCC dialect, no libc. */
package runtime;

void *memcpy(void *dst, const void *src, size_t n) {
    char *d = (char *)dst;
    const char *s = (const char *)src;
    size_t i = 0;
    while (i < n) {
        d[i] = s[i];
        i = i + 1;
    }
    return dst;
}

void *memmove(void *dst, const void *src, size_t n) {
    char *d = (char *)dst;
    const char *s = (const char *)src;
    if (d == s) return dst;
    if ((unsigned long)d < (unsigned long)s) {
        size_t i = 0;
        while (i < n) {
            d[i] = s[i];
            i = i + 1;
        }
    } else {
        size_t i = n;
        while (i > 0) {
            i = i - 1;
            d[i] = s[i];
        }
    }
    return dst;
}

void *memset(void *dst, int c, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    unsigned char b = (unsigned char)c;
    size_t i = 0;
    while (i < n) {
        d[i] = b;
        i = i + 1;
    }
    return dst;
}

int memcmp(const void *a, const void *b, size_t n) {
    const unsigned char *x = (const unsigned char *)a;
    const unsigned char *y = (const unsigned char *)b;
    size_t i = 0;
    while (i < n) {
        if (x[i] != y[i]) return (int)x[i] - (int)y[i];
        i = i + 1;
    }
    return 0;
}

size_t strlen(const char *s) {
    size_t n = 0;
    while (s[n] != 0) n = n + 1;
    return n;
}

int strcmp(const char *a, const char *b) {
    size_t i = 0;
    while (a[i] != 0 && a[i] == b[i]) i = i + 1;
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
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
