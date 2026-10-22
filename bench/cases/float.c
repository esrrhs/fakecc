/* Loop-carried float arithmetic. Values stay dyadic so rounding matches. */
package main;
import runtime;

enum { N = 20000000 };

int main(void) {
    int i;
    float x;
    int scaled;

    x = 1.0f;
    for (i = 0; i < N; i++)
        x = (x + (float)(i & 15)) * 0.5f;
    if (x < 0.0f)
        x = -x;
    scaled = (int)(x * 1024.0f);
    runtime.printf("float %d\n", scaled);
    return 0;
}
