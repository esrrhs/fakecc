// runtime.fputc / runtime.fputs: write characters and a string to a file and read them
// back.  runtime.fputc writes one byte at a time; runtime.fputs writes a whole string
// without a trailing newline.
// expect: 0
package main;

import runtime;
int main() {
    char buf[16];
    runtime.FILE *f;
    long n;

    f = runtime.fopen("/tmp/fakecc_fputc_test.dat", "w");
    if (f == 0) return 1;

    if (runtime.fputc('X', f) < 0) { runtime.fclose(f); return 2; }
    if (runtime.fputc('Y', f) < 0) { runtime.fclose(f); return 3; }
    if (runtime.fputs("hello", f) < 0) { runtime.fclose(f); return 4; }

    if (runtime.fclose(f) != 0) return 5;

    f = runtime.fopen("/tmp/fakecc_fputc_test.dat", "r");
    if (f == 0) return 6;
    n = runtime.fread(buf, 1, 16, f);
    if (n != 7) { runtime.fclose(f); return 7; } /* X Y h e l l o */
    if (buf[0] != 'X') { runtime.fclose(f); return 8; }
    if (buf[1] != 'Y') { runtime.fclose(f); return 9; }
    if (buf[2] != 'h' || buf[3] != 'e' || buf[4] != 'l' || buf[5] != 'l' || buf[6] != 'o') {
        runtime.fclose(f); return 10;
    }
    if (runtime.fclose(f) != 0) return 11;
    return 0;
}
