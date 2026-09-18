// expect: 0
// File-scope, static-local, automatic, and TLS objects with aligned(16)
// must actually be 16-byte aligned at runtime (after a leading char).
package main;

char pad = 1;
int g __attribute__((aligned(16))) = 7;

__thread char tc = 1;
__thread int tx __attribute__((aligned(16)));

int main(void) {
    char c;
    int loc __attribute__((aligned(16)));
    static char sc;
    static int sl __attribute__((aligned(16)));

    loc = 3;
    sl = 4;
    tx = 5;
    c = 1;
    sc = 1;
    (void)c;
    (void)sc;
    (void)pad;
    if (((unsigned long)&g & 15ul) != 0) return 1;
    if (((unsigned long)&loc & 15ul) != 0) return 2;
    if (((unsigned long)&sl & 15ul) != 0) return 3;
    if (((unsigned long)&tx & 15ul) != 0) return 4;
    if (g != 7) return 5;
    if (loc != 3) return 6;
    if (sl != 4) return 7;
    if (tx != 5) return 8;
    return 0;
}
