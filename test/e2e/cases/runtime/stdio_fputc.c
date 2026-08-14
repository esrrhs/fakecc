// io.fputc / io.fputs: write characters and a string to a file and read them
// back.  io.fputc writes one byte at a time; io.fputs writes a whole string
// without a trailing newline.
// expect: 0
package main;

import io;
int main() {
    char buf[16];
    io.FILE *f;
    long n;

    f = io.fopen("/tmp/fakecc_fputc_test.dat", "w");
    if (f == 0) return 1;

    if (io.fputc('X', f) < 0) { io.fclose(f); return 2; }
    if (io.fputc('Y', f) < 0) { io.fclose(f); return 3; }
    if (io.fputs("hello", f) < 0) { io.fclose(f); return 4; }

    if (io.fclose(f) != 0) return 5;

    f = io.fopen("/tmp/fakecc_fputc_test.dat", "r");
    if (f == 0) return 6;
    n = io.fread(buf, 1, 16, f);
    if (n != 7) { io.fclose(f); return 7; } /* X Y h e l l o */
    if (buf[0] != 'X') { io.fclose(f); return 8; }
    if (buf[1] != 'Y') { io.fclose(f); return 9; }
    if (buf[2] != 'h' || buf[3] != 'e' || buf[4] != 'l' || buf[5] != 'l' || buf[6] != 'o') {
        io.fclose(f); return 10;
    }
    if (io.fclose(f) != 0) return 11;
    return 0;
}
