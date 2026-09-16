// expect: 0
// fread must consume a prior ungetc, and fopen("rb+") must be readable.
package main;
import runtime;
int main(void) {
    runtime.FILE *f;
    char buf[8];
    long n;

    f = runtime.fopen("/tmp/fakecc_ungetc_fread.dat", "w");
    if (f == 0) return 1;
    if (runtime.fwrite("abcd", 1, 4, f) != 4) { runtime.fclose(f); return 2; }
    runtime.fclose(f);

    f = runtime.fopen("/tmp/fakecc_ungetc_fread.dat", "rb+");
    if (f == 0) return 3;
    if (runtime.fgetc(f) != 'a') { runtime.fclose(f); return 4; }
    if (runtime.ungetc('Z', f) != 'Z') { runtime.fclose(f); return 5; }
    n = runtime.fread(buf, 1, 4, f);
    if (n != 4) { runtime.fclose(f); return 6; }
    if (buf[0] != 'Z' || buf[1] != 'b' || buf[2] != 'c' || buf[3] != 'd') {
        runtime.fclose(f); return 7;
    }
    runtime.fclose(f);
    return 0;
}
