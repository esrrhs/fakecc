// runtime.chmod: sets a file's mode bits via sys_chmod.  Pin that it returns
// 0 on a real file and -1 on a path that does not exist.  (The exact mode is
// not re-checked: fakecc has no stat, and the point here is the syscall
// wrapper's success/failure reporting.)
// expect: 0
package main;
import runtime;
int main() {
    /* create a real file to chmod */
    runtime.FILE *f = runtime.fopen("/tmp/fakecc_chmod_test.dat", "w");
    if (f == 0) return 1;
    runtime.fclose(f);

    /* success path returns 0 */
    if (runtime.chmod("/tmp/fakecc_chmod_test.dat", 755) != 0) return 2;

    /* failure path: no such file returns -1 */
    if (runtime.chmod("/tmp/fakecc_chmod_does_not_exist_xyz", 644) != -1) return 3;

    return 0;
}
