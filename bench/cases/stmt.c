/* Statement expressions and typeof in the inner loop. */
package main;
import runtime;

enum { N = 10000000 };

int main(void) {
    unsigned i;
    int x;
    unsigned acc;

    x = 1;
    acc = 0;
    for (i = 0; i < N; i++) {
        typeof(x) y = ({
            int t = x * 3 + (int)(i & 7u);
            t ^ (int)(i & 15u);
        });
        typeof(i) step = i & 3u;
        x = y + (int)step;
        acc += (unsigned)x;
    }
    runtime.printf("stmt %u\n", acc);
    return 0;
}
