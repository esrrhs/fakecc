// stdio file I/O: fopen/fclose/fwrite/fread/fseek/ftell over a real
// file in a temporary location.  Pin that bytes written can be read
// back, that fseek+ftell track the offset, and that fileno returns the
// expected descriptor numbers for the standard streams.
// expect: 0
package main;
typedef struct FILE FILE;
extern FILE *fopen(const char *path, const char *mode);
extern int fclose(FILE *f);
extern long fwrite(const void *p, long sz, long nm, FILE *f);
extern long fread(void *p, long sz, long nm, FILE *f);
extern int fseek(FILE *f, long off, int whence);
extern long ftell(FILE *f);
extern int fileno(FILE *f);
extern FILE *stdin;
extern FILE *stdout;
extern FILE *stderr;

int main() {
    char buf[16];
    long n;
    FILE *f;

    /* write a file */
    f = fopen("/tmp/fakecc_stdio_test.dat", "w");
    if (f == 0) return 1;
    if (fwrite("abcd", 1, 4, f) != 4) { fclose(f); return 2; }
    if (fclose(f) != 0) return 3;

    /* read it back */
    f = fopen("/tmp/fakecc_stdio_test.dat", "r");
    if (f == 0) return 4;
    n = fread(buf, 1, 16, f);
    if (n != 4) { fclose(f); return 5; }
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c' || buf[3] != 'd') {
        fclose(f); return 6;
    }

    /* fseek to offset 1 (SEEK_SET=0), ftell reports 1, next read is 'b' */
    if (fseek(f, 1, 0) != 0) { fclose(f); return 7; }
    if (ftell(f) != 1) { fclose(f); return 8; }
    n = fread(buf, 1, 1, f);
    if (n != 1 || buf[0] != 'b') { fclose(f); return 9; }

    if (fclose(f) != 0) return 10;

    /* fileno of the standard streams */
    if (fileno(stdin) != 0) return 11;
    if (fileno(stdout) != 1) return 12;
    if (fileno(stderr) != 2) return 13;

    return 0;
}
