// expect: 0
/* One import covers the whole runtime package. */
package main;
import runtime;
int main(void) {
    runtime.printf("ok\n");
    return 0;
}
