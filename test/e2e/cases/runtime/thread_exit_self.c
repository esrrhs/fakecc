// expect: 0
// gcc_flags: -pthread
package main;

import runtime;

void *worker(void *arg) {
    runtime.pthread_t self = runtime.pthread_self();
    if (self == 0) runtime.pthread_exit((void *)1);
    runtime.pthread_exit((void *)77);
    return (void *)99;
}

int main() {
    runtime.pthread_t t1;
    void *ret = 0;

    int r = runtime.pthread_create(&t1, 0, worker, 0);
    if (r != 0) return 1;

    runtime.pthread_join(t1, &ret);
    if ((long)ret != 77) return 2;

    return 0;
}
