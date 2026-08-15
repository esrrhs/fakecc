// runtime.fprintf to a real file (not stdout): format mixed types, close,
// reopen for reading, and verify the bytes written.  import_file_type.c and
// import_stdout.c only exercise fprintf to runtime.stdout; this pins the
// vfprintf-to-fputc path against a ordinary FILE.
// expect: 0
package main;
import runtime;
int main() {
    runtime.FILE *f = runtime.fopen("/tmp/fakecc_fprintf_test.dat", "w");
    if (f == 0) return 1;

    /* "x=7:hi" is 6 chars */
    int n = runtime.fprintf(f, "x=%d:%s", 7, "hi");
    if (n != 6) { runtime.fclose(f); return 2; }

    if (runtime.fclose(f) != 0) return 3;

    f = runtime.fopen("/tmp/fakecc_fprintf_test.dat", "r");
    if (f == 0) return 4;
    char buf[16];
    long r = runtime.fread(buf, 1, 16, f);
    if (r != 6) { runtime.fclose(f); return 5; }
    buf[6] = 0;
    if (runtime.strcmp(buf, "x=7:hi") != 0) { runtime.fclose(f); return 6; }
    if (runtime.fclose(f) != 0) return 7;

    return 0;
}
