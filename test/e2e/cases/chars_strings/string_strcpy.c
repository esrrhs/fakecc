// runtime.strcpy: copies src including its \0 terminator, returns dst.  A
// loop that stops early or forgets the terminator leaves dst without
// a \0, so runtime.strlen(dst) is the real check.  Also verify that
// overwriting with a shorter string clears the old tail.
// expect: 0
package main;
import runtime;
int main() {
    char buf[16];
    char *r;

    r = runtime.strcpy(buf, "hello");
    if (r != buf) return 1;
    if (runtime.strlen(buf) != 5) return 2;
    if (buf[0] != 'h' || buf[4] != 'o' || buf[5] != 0) return 3;

    /* overwrite with a shorter string: the old tail must be gone */
    r = runtime.strcpy(buf, "hi");
    if (r != buf) return 4;
    if (runtime.strlen(buf) != 2) return 5;
    if (buf[2] != 0) return 6;
    return 0;
}
