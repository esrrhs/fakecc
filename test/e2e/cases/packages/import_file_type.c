// expect: 0
package main;
import runtime;
int main(void) {
    runtime.FILE *f = runtime.stdout;
    runtime.fprintf(f, "x\n");
    return 0;
}
