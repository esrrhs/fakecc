// stdio file I/O: io.fopen/io.fclose/io.fwrite/io.fread/io.fseek/io.ftell over a real
// file in a temporary location.  Pin that bytes written can be read
// back, that io.fseek+io.ftell track the offset, and that io.fileno returns the
// expected descriptor numbers for the standard streams.
// expect: 0
package main;

import io;
int main() {
    char buf[16];
    long n;
    io.FILE *f;

    /* write a file */
    f = io.fopen("/tmp/fakecc_stdio_test.dat", "w");
    if (f == 0) return 1;
    if (io.fwrite("abcd", 1, 4, f) != 4) { io.fclose(f); return 2; }
    if (io.fclose(f) != 0) return 3;

    /* read it back */
    f = io.fopen("/tmp/fakecc_stdio_test.dat", "r");
    if (f == 0) return 4;
    n = io.fread(buf, 1, 16, f);
    if (n != 4) { io.fclose(f); return 5; }
    if (buf[0] != 'a' || buf[1] != 'b' || buf[2] != 'c' || buf[3] != 'd') {
        io.fclose(f); return 6;
    }

    /* io.fseek to offset 1 (SEEK_SET=0), io.ftell reports 1, next read is 'b' */
    if (io.fseek(f, 1, 0) != 0) { io.fclose(f); return 7; }
    if (io.ftell(f) != 1) { io.fclose(f); return 8; }
    n = io.fread(buf, 1, 1, f);
    if (n != 1 || buf[0] != 'b') { io.fclose(f); return 9; }

    if (io.fclose(f) != 0) return 10;

    /* io.fileno of the standard streams */
    if (io.fileno(io.stdin) != 0) return 11;
    if (io.fileno(io.stdout) != 1) return 12;
    if (io.fileno(io.stderr) != 2) return 13;

    return 0;
}
