// expect: 0
// remove() returns 0 on success and -1 on failure (not a raw -errno).
package main;
import runtime;
int main(void) {
    const char *path = "/tmp/fcc_remove_test.txt";
    runtime.FILE *f = runtime.fopen(path, "w");
    if (!f) return 1;
    runtime.fputs("x", f);
    runtime.fclose(f);
    if (runtime.remove(path) != 0) return 2;
    if (runtime.remove(path) != -1) return 3;
    if (runtime.remove("/tmp/fcc_remove_no_such_file_xyz") != -1) return 4;
    return 0;
}
