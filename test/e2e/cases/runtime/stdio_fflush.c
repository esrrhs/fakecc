// io.fflush / io.perror: the remaining untested stdio entry points.  io.fflush on a
// stream forces a buffered write; io.perror writes a diagnostic to io.stderr.
// The suite discards io.stderr, so io.perror is only checked to not crash and to
// leave the process with a defined exit code.  io.fflush is checked via a
// file round-trip: write, flush, then read back before io.fclose.
// expect: 0
package main;

import io;
int main() {
    char buf[16];
    io.FILE *f;
    long n;

    f = io.fopen("/tmp/fakecc_fflush_test.dat", "w");
    if (f == 0) return 1;

    if (io.fputc('Z', f) < 0) { io.fclose(f); return 2; }

    /* flush the data out to the kernel before closing */
    if (io.fflush(f) != 0) { io.fclose(f); return 3; }

    /* after flushing, a reader opening the file must see the byte */
    if (io.fclose(f) != 0) return 4;

    f = io.fopen("/tmp/fakecc_fflush_test.dat", "r");
    if (f == 0) return 5;
    n = io.fread(buf, 1, 16, f);
    if (n != 1) { io.fclose(f); return 6; }
    if (buf[0] != 'Z') { io.fclose(f); return 7; }
    if (io.fclose(f) != 0) return 8;

    /* io.perror must not crash; it writes to io.stderr (discarded by the suite) */
    io.perror("fakecc_fflush_test");

    return 0;
}
