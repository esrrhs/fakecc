// expect: 42
// flags: -fsanitize=address
// skip_oracle: clang
package main;
import runtime;

int main(void) {
    char *s = (char *)runtime.malloc(32);
    char *t = (char *)runtime.malloc(32);
    s = (char *)runtime.realloc(s, 19);
    t = (char *)runtime.realloc(t, 19);
    for (int i = 0; i < 18; i = i + 1) {
        s[i] = 'a';
        t[i] = 'a';
    }
    s[18] = 0;
    t[18] = 0;
    if (runtime.strlen(s) != 18) return 1;
    if (runtime.strcmp(s, t) != 0) return 2;
    runtime.free(t);
    runtime.free(s);

    /* Non-split recycled chunk handed to malloc(0) then grown via realloc. */
    void *a = runtime.malloc(64);
    void *pin = runtime.malloc(16);
    runtime.free(a);
    void *p = runtime.malloc(0);
    void *q = runtime.realloc(p, 128);
    if (q == 0) return 3;
    runtime.free(q);
    runtime.free(pin);
    return 42;
}
