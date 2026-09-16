// expect: 0
// ftell after ungetc reports the logical position (kernel offset minus pushback).
package main;
import runtime;
int main(void) {
    const char *path = "/tmp/fcc_ftell_ungetc.txt";
    runtime.FILE *f = runtime.fopen(path, "w+");
    if (!f) return 1;
    runtime.fputs("abcd", f);
    runtime.fseek(f, 0, 0);
    int c = runtime.fgetc(f);
    if (c != 'a') return 2;
    long p1 = runtime.ftell(f);
    if (p1 != 1) return 3;
    runtime.ungetc(c, f);
    long p0 = runtime.ftell(f);
    if (p0 != 0) return 4;
    runtime.fclose(f);
    runtime.remove(path);
    return 0;
}
