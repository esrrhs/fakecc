// expect: 0
// fflush(NULL) and exit() must flush fopen'd streams, not only stdout/stderr.
package main;
import runtime;
int main(void) {
    runtime.FILE *f;
    char buf[16];
    long n;

    f = runtime.fopen("/tmp/fakecc_fflush_all.dat", "w");
    if (f == 0) return 1;
    if (runtime.fwrite("hello", 1, 5, f) != 5) { runtime.fclose(f); return 2; }
    if (runtime.fflush(0) != 0) { runtime.fclose(f); return 3; }
    runtime.fclose(f);

    f = runtime.fopen("/tmp/fakecc_fflush_all.dat", "r");
    if (f == 0) return 4;
    n = runtime.fread(buf, 1, 16, f);
    if (n != 5) { runtime.fclose(f); return 5; }
    if (buf[0] != 'h' || buf[4] != 'o') { runtime.fclose(f); return 6; }
    runtime.fclose(f);

    if (runtime.fwrite("x", 0, 10, runtime.stdout) != 0) return 7;
    return 0;
}
