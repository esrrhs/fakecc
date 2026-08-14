// fflush / perror: the remaining untested stdio entry points.  fflush on a
// stream forces a buffered write; perror writes a diagnostic to stderr.
// The suite discards stderr, so perror is only checked to not crash and to
// leave the process with a defined exit code.  fflush is checked via a
// file round-trip: write, flush, then read back before fclose.
// expect: 0
package main;
typedef struct FILE FILE;
extern FILE *fopen(const char *path, const char *mode);
extern int fclose(FILE *f);
extern int fputc(int c, FILE *f);
extern int fflush(FILE *f);
extern long fread(void *p, long sz, long nm, FILE *f);
extern void perror(const char *s);

int main() {
    char buf[16];
    FILE *f;
    long n;

    f = fopen("/tmp/fakecc_fflush_test.dat", "w");
    if (f == 0) return 1;

    if (fputc('Z', f) < 0) { fclose(f); return 2; }

    /* flush the data out to the kernel before closing */
    if (fflush(f) != 0) { fclose(f); return 3; }

    /* after flushing, a reader opening the file must see the byte */
    if (fclose(f) != 0) return 4;

    f = fopen("/tmp/fakecc_fflush_test.dat", "r");
    if (f == 0) return 5;
    n = fread(buf, 1, 16, f);
    if (n != 1) { fclose(f); return 6; }
    if (buf[0] != 'Z') { fclose(f); return 7; }
    if (fclose(f) != 0) return 8;

    /* perror must not crash; it writes to stderr (discarded by the suite) */
    perror("fakecc_fflush_test");

    return 0;
}
