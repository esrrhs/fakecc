// expect: 0
package main;
import io;
import fmt;
int main(void) {
    io.__rt_stdio_init();
    io.FILE *f = io.stdout;
    fmt.fprintf(f, "x\n");
    return 0;
}
