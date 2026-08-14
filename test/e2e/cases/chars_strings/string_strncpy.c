// strncpy: copies up to n bytes; if src is shorter than n, pads the
// rest with \0.  The padding behaviour is the easy one to get wrong.
// Verify the pad, the exact-n case (no pad, no \0), and the
// longer-than-n case (truncation).
// expect: 0
package main;
extern char *strncpy(char *dst, const char *src, unsigned long n);
int main() {
    char buf[8];
    int i;
    char *r;

    /* poison the whole buffer */
    i = 0;
    while (i < 8) { buf[i] = 'X'; i = i + 1; }

    /* src shorter than n: copy "ab" into 8 bytes, expect 6 \0 pads */
    r = strncpy(buf, "ab", 8);
    if (r != buf) return 1;
    if (buf[0] != 'a' || buf[1] != 'b') return 2;
    i = 2;
    while (i < 8) { if (buf[i] != 0) return 3; i = i + 1; }

    /* src exactly n: no padding, no \0 terminator copied */
    i = 0;
    while (i < 8) { buf[i] = 'X'; i = i + 1; }
    r = strncpy(buf, "abcdefgh", 8);
    if (r != buf) return 4;
    if (buf[0] != 'a' || buf[7] != 'h') return 5;

    /* src longer than n: only n bytes copied */
    i = 0;
    while (i < 8) { buf[i] = 'X'; i = i + 1; }
    r = strncpy(buf, "abcdefghij", 8);
    if (r != buf) return 6;
    if (buf[0] != 'a' || buf[7] != 'h') return 7;

    return 0;
}
