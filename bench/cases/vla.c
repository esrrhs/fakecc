/* A VLA allocated on each call, then used as a sliding window. */
package main;
import runtime;

enum { SRC = 4096 };
enum { CALLS = 4000 };

static int src[SRC];

static unsigned blur(int n, const int *in) {
    int buf[n];
    int i, k;
    unsigned s;

    for (i = 0; i < n; i++)
        buf[i] = in[i];
    for (k = 0; k < 24; k++) {
        int prev = buf[0];
        for (i = 1; i < n - 1; i++) {
            int cur = buf[i];
            buf[i] = (prev + cur * 3 + buf[i + 1]) & 0xffff;
            prev = cur;
        }
    }
    s = 0;
    for (i = 0; i < n; i++)
        s += (unsigned)buf[i];
    return s;
}

int main(void) {
    int i, c;
    unsigned acc;

    for (i = 0; i < SRC; i++)
        src[i] = (i * 17 + 5) & 0xff;
    acc = 0;
    for (c = 0; c < CALLS; c++) {
        int n = 64 + (c % 5) * 48;
        acc += blur(n, src + (c % 32));
    }
    runtime.printf("vla %u\n", acc);
    return 0;
}
