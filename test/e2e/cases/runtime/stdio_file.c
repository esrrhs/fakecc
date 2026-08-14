// stdio file I/O: runtime.fopen/runtime.fclose/runtime.fwrite/runtime.fread/runtime.fseek/runtime.ftell over a real
// file in a temporary location.  Pin that bytes written can be read
// back, that runtime.fseek+runtime.ftell track the offset, and that runtime.fileno returns the
// expected descriptor numbers for the standard streams.
// expect: 0
package main;

import runtime;
int main() {
    char buf[16];
    long n;
    runtime.FILE *f;

    /* write a file */
    f = runtime.fopen("/tmp/fakecc_stdio_test.dat", "w");
    if (f == 0) return 1;
    if (runtime.fwrite("abcd", 1, 4, f) != 4) { runtime.fclose(f); return 2; }
    if (runtime.fclose(f) != 0) return 3;

    /* read it back */
    f = runtime.fopen("/tmp/fakecc_stdio_test.dat", "r");
    if (f == 0) return 4;
    n = runtime.fread(buf, 1, 16, f);
    if (n != 4) { runtime.fclose(f); return 5; }
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c' || buf[3] != 'd') {
        runtime.fclose(f); return 6;
    }

    /* runtime.fseek to offset 1 (SEEK_SET=0), runtime.ftell reports 1, next read is 'b' */
    if (runtime.fseek(f, 1, 0) != 0) { runtime.fclose(f); return 7; }
    if (runtime.ftell(f) != 1) { runtime.fclose(f); return 8; }
    n = runtime.fread(buf, 1, 1, f);
    if (n != 1 || buf[0] != 'b') { runtime.fclose(f); return 9; }

    if (runtime.fclose(f) != 0) return 10;

    /* runtime.fileno of the standard streams */
    if (runtime.fileno(runtime.stdin) != 0) return 11;
    if (runtime.fileno(runtime.stdout) != 1) return 12;
    if (runtime.fileno(runtime.stderr) != 2) return 13;

    return 0;
}
