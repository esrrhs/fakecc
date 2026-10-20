/* Hand-rolled copy, compare, and hash. No libc besides printf. */
package main;
import runtime;

enum { NSTR = 4000 };
enum { LEN = 48 };
enum { PASSES = 30 };

static char buf[NSTR][LEN];

static int slen(const char *s) {
    int n = 0;
    while (s[n] != 0)
        n++;
    return n;
}

static void scopy(char *d, const char *s) {
    int i = 0;
    while (s[i] != 0) {
        d[i] = s[i];
        i++;
    }
    d[i] = 0;
}

static int scmp(const char *a, const char *b) {
    int i = 0;
    while (a[i] != 0 && a[i] == b[i])
        i++;
    return (int)(unsigned char)a[i] - (int)(unsigned char)b[i];
}

static unsigned shash(const char *s) {
    unsigned h = 2166136261u;
    int i = 0;
    while (s[i] != 0) {
        h ^= (unsigned char)s[i];
        h *= 16777619u;
        i++;
    }
    return h;
}

int main(void) {
    int i, p;
    unsigned acc;

    for (i = 0; i < NSTR; i++) {
        int k;
        for (k = 0; k < LEN - 1; k++)
            buf[i][k] = (char)('a' + ((i * 3 + k * 5) % 26));
        buf[i][LEN - 1] = 0;
    }
    acc = 0;
    for (p = 0; p < PASSES; p++) {
        for (i = 0; i < NSTR; i++) {
            char tmp[LEN];
            int j = (i * 17 + p) % NSTR;
            scopy(tmp, buf[i]);
            acc += (unsigned)slen(tmp);
            acc += shash(tmp);
            if (scmp(tmp, buf[j]) < 0)
                acc += 1u;
            tmp[p % (LEN - 1)] = (char)('a' + (p % 26));
            scopy(buf[i], tmp);
        }
    }
    runtime.printf("string %u\n", acc);
    return 0;
}
