// fputc / fputs: write characters and a string to a file and read them
// back.  fputc writes one byte at a time; fputs writes a whole string
// without a trailing newline.
// expect: 0
package main;
typedef struct FILE FILE;
extern FILE *fopen(const char *path, const char *mode);
extern int fclose(FILE *f);
extern int fputc(int c, FILE *f);
extern int fputs(const char *s, FILE *f);
extern long fread(void *p, long sz, long nm, FILE *f);

int main() {
    char buf[16];
    FILE *f;
    long n;

    f = fopen("/tmp/fakecc_fputc_test.dat", "w");
    if (f == 0) return 1;

    if (fputc('X', f) < 0) { fclose(f); return 2; }
    if (fputc('Y', f) < 0) { fclose(f); return 3; }
    if (fputs("hello", f) < 0) { fclose(f); return 4; }

    if (fclose(f) != 0) return 5;

    f = fopen("/tmp/fakecc_fputc_test.dat", "r");
    if (f == 0) return 6;
    n = fread(buf, 1, 16, f);
    if (n != 7) { fclose(f); return 7; } /* X Y h e l l o */
    if (buf[0] != 'X') { fclose(f); return 8; }
    if (buf[1] != 'Y') { fclose(f); return 9; }
    if (buf[2] != 'h' || buf[3] != 'e' || buf[4] != 'l' || buf[5] != 'l' || buf[6] != 'o') {
        fclose(f); return 10;
    }
    if (fclose(f) != 0) return 11;
    return 0;
}
