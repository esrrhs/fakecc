// expect: 0
// gcc_flags: -pthread
package main;

import runtime;

void *add_ten(void *arg) {
    long x = (long)arg;
    return (void *)(x + 10);
}

int main() {
    runtime.thread_t th;
    void *ret = 0;

    int r = runtime.thread_create(&th, add_ten, (void *)32);
    if (r != 0) return 1;

    runtime.thread_join(th, &ret);
    if ((long)ret != 42) return 2;

    return 0;
}
