// expect: 0
package main;

const unsigned long CLONE_VM        = 0x00000100;
const unsigned long CLONE_FS        = 0x00000200;
const unsigned long CLONE_FILES     = 0x00000400;
const unsigned long CLONE_SIGHAND   = 0x00000800;
const unsigned long CLONE_THREAD    = 0x00010000;
const unsigned long CLONE_SYSVSEM   = 0x00040000;
const unsigned long CLONE_SETTLS    = 0x00080000;

__thread int thread_id;
__thread int thread_accum;

static char stack1[8192];
static char stack2[8192];
static char tls_area1[1024];
static char tls_area2[1024];

volatile int t1_done;
volatile int t2_done;
int t1_result;
int t2_result;

void worker1() {
    thread_id = 1;
    thread_accum = 0;
    for (int i = 0; i < 100; i++) thread_accum += 1;
    t1_result = thread_accum;
    t1_done = 1;
    __syscall(60, 0);
}

void worker2() {
    thread_id = 2;
    thread_accum = 0;
    for (int i = 0; i < 100; i++) thread_accum += 2;
    t2_result = thread_accum;
    t2_done = 1;
    __syscall(60, 0);
}

int main() {
    t1_done = 0;
    t2_done = 0;
    thread_id = 0;
    thread_accum = 9999;

    unsigned long *tcb1 = (unsigned long *)(tls_area1 + 512);
    *tcb1 = (unsigned long)tcb1;
    unsigned long *tcb2 = (unsigned long *)(tls_area2 + 512);
    *tcb2 = (unsigned long)tcb2;

    unsigned long flags = CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS;

    long ret1 = __syscall(56, flags, (long)(stack1 + 8192 - 16), 0, 0, (long)tcb1);
    if (ret1 == 0) worker1();

    long ret2 = __syscall(56, flags, (long)(stack2 + 8192 - 16), 0, 0, (long)tcb2);
    if (ret2 == 0) worker2();

    while (!t1_done || !t2_done) {
    }

    if (t1_result != 100) return 1;
    if (t2_result != 200) return 2;
    if (thread_id != 0) return 3;
    if (thread_accum != 9999) return 4;

    return 0;
}
