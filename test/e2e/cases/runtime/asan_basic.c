// expect: 42
// flags: -fsanitize=address
package main;
import runtime;

int main() {
    int *p = (int *)runtime.malloc(16 * sizeof(int));
    for (int i = 0; i < 16; i++) {
        p[i] = i * 2;
    }
    int sum = 0;
    for (int i = 0; i < 16; i++) {
        sum += p[i];
    }
    runtime.free(p);
    return 42;
}
