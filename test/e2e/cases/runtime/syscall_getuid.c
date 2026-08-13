// expect: 0
// getuid() returns unsigned uid; on typical test environments this is
// non-negative so we return 0 unconditionally after touching the syscall.
package main;
int main() {
    long uid = __syscall(102);   // getuid
    if (uid < 0) { return 1; }
    return 0;
}
