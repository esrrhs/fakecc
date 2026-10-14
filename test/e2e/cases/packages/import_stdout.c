// expect: 0
package main;
// skip_oracle: clang
import runtime;
int main(void) {
    runtime.fprintf(runtime.stdout, "hi\n");
    return 0;
}
