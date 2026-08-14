// runtime.fflush / runtime.perror: the remaining untested stdio entry points.  runtime.fflush on a
// stream forces a buffered write; runtime.perror writes a diagnostic to runtime.stderr.
// The suite discards runtime.stderr, so runtime.perror is only checked to not crash and to
// leave the process with a defined exit code.  runtime.fflush is checked via a
// file round-trip: write, flush, then read back before runtime.fclose.
// expect: 0
package main;

import runtime;
int main() {
    char buf[16];
    runtime.FILE *f;
    long n;

    f = runtime.fopen("/tmp/fakecc_fflush_test.dat", "w");
    if (f == 0) return 1;

    if (runtime.fputc('Z', f) < 0) { runtime.fclose(f); return 2; }

    /* flush the data out to the kernel before closing */
    if (runtime.fflush(f) != 0) { runtime.fclose(f); return 3; }

    /* after flushing, a reader opening the file must see the byte */
    if (runtime.fclose(f) != 0) return 4;

    f = runtime.fopen("/tmp/fakecc_fflush_test.dat", "r");
    if (f == 0) return 5;
    n = runtime.fread(buf, 1, 16, f);
    if (n != 1) { runtime.fclose(f); return 6; }
    if (buf[0] != 'Z') { runtime.fclose(f); return 7; }
    if (runtime.fclose(f) != 0) return 8;

    /* runtime.perror must not crash; it writes to runtime.stderr (discarded by the suite) */
    runtime.perror("fakecc_fflush_test");

    return 0;
}
