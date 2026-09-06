/* POSIX-compatible and lightweight thread runtime — FakeCC dialect. */
package runtime;

typedef unsigned long pthread_t;
typedef unsigned long thread_t;

typedef struct {
    int __detachstate;
    size_t __stacksize;
} pthread_attr_t;

static const unsigned long FAKECC_THREAD_MAGIC = 0x54485244;
static const unsigned long FAKECC_STACK_SIZE = 2097152; /* 2MB aligned stack per thread */

struct __fakecc_thread {
    unsigned long magic;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    void *stack_base;
    size_t stack_size;
    void *tls_base;
    size_t tls_size;
    volatile int done;
    volatile int joined;
    int tid;
};

void *__fakecc_tls_image = 0;
unsigned long __fakecc_tls_filesz = 0;
unsigned long __fakecc_tls_memsz = 0;
unsigned long __fakecc_tls_align = 16;

static struct __fakecc_thread __main_thread;

static void *__fakecc_thread_trampoline(void *arg) {
    struct __fakecc_thread *t = (struct __fakecc_thread *)arg;
    t->tid = (int)__syscall(186 /* sys_gettid */);
    void *res = t->start_routine(t->arg);
    t->retval = res;
    t->done = 1;
    __syscall(202, (long)&t->done, 1 /* FUTEX_WAKE */, 1, 0, 0, 0);
    return res;
}

pthread_t pthread_self(void) {
    int tid = (int)__syscall(186 /* sys_gettid */);
    int pid = (int)__syscall(39 /* sys_getpid */);
    if (tid == pid) {
        __main_thread.tid = pid;
        return (pthread_t)&__main_thread;
    }
    int dummy;
    unsigned long sp = (unsigned long)&dummy;
    unsigned long base = sp & ~(FAKECC_STACK_SIZE - 1);
    struct __fakecc_thread *t = (struct __fakecc_thread *)base;
    if (t->magic == FAKECC_THREAD_MAGIC) {
        return (pthread_t)t;
    }
    return (pthread_t)&__main_thread;
}

void pthread_exit(void *retval) {
    struct __fakecc_thread *t = (struct __fakecc_thread *)pthread_self();
    if (t && t != &__main_thread) {
        t->retval = retval;
        t->done = 1;
        __syscall(202, (long)&t->done, 1 /* FUTEX_WAKE */, 1, 0, 0, 0);
    }
    __syscall(60, 0);
}

void thread_exit(void *retval) {
    pthread_exit(retval);
}

thread_t thread_self(void) {
    return (thread_t)pthread_self();
}

int pthread_create(pthread_t *thread, const void *attr, void *(*start_routine)(void *), void *arg) {
    if (!thread || !start_routine) return -1;

    /* Allocate 4MB, trim to 2MB-aligned 2MB stack */
    size_t alloc_size = FAKECC_STACK_SIZE * 2;
    void *raw = (void *)__syscall(9, 0, (long)alloc_size, 3 /* PROT_READ|PROT_WRITE */, 0x22 /* MAP_PRIVATE|MAP_ANONYMOUS */, -1, 0);
    if ((long)raw < 0) return -1;

    unsigned long aligned = ((unsigned long)raw + FAKECC_STACK_SIZE - 1) & ~(FAKECC_STACK_SIZE - 1);
    if (aligned > (unsigned long)raw) {
        __syscall(11, (long)raw, (long)(aligned - (unsigned long)raw));
    }
    unsigned long end = aligned + FAKECC_STACK_SIZE;
    unsigned long raw_end = (unsigned long)raw + alloc_size;
    if (raw_end > end) {
        __syscall(11, (long)end, (long)(raw_end - end));
    }

    void *stack_base = (void *)aligned;
    size_t stack_size = FAKECC_STACK_SIZE;

    size_t tls_memsz = __fakecc_tls_memsz;
    size_t tls_pad = 0;
    while ((tls_memsz + tls_pad) & 15) tls_pad++;
    size_t tcb_offset = tls_memsz + tls_pad;
    if (tcb_offset < 16) tcb_offset = 16;
    size_t tls_alloc = (tcb_offset + 64 + 4095) & ~4095UL;
    void *tls_base = (void *)__syscall(9, 0, (long)tls_alloc, 3, 0x22, -1, 0);
    if ((long)tls_base < 0) {
        __syscall(11, (long)stack_base, (long)stack_size);
        return -1;
    }

    char *tls_ptr = (char *)tls_base;
    unsigned long tcb = (unsigned long)(tls_ptr + tcb_offset);
    char *init_tls = (char *)(tcb - tls_memsz);
    if (__fakecc_tls_filesz > 0 && __fakecc_tls_image) {
        memcpy(init_tls, __fakecc_tls_image, __fakecc_tls_filesz);
    }
    if (tls_memsz > __fakecc_tls_filesz) {
        memset(init_tls + __fakecc_tls_filesz, 0, tls_memsz - __fakecc_tls_filesz);
    }
    *(unsigned long *)tcb = tcb; /* %fs:0 -> tcb */

    struct __fakecc_thread *t = (struct __fakecc_thread *)stack_base;
    t->magic = FAKECC_THREAD_MAGIC;
    t->start_routine = start_routine;
    t->arg = arg;
    t->retval = 0;
    t->stack_base = stack_base;
    t->stack_size = stack_size;
    t->tls_base = tls_base;
    t->tls_size = tls_alloc;
    t->done = 0;
    t->joined = 0;

    /* CLONE_VM | CLONE_FS | CLONE_FILES | CLONE_SIGHAND | CLONE_THREAD | CLONE_SYSVSEM | CLONE_SETTLS | CLONE_PARENT_SETTID | CLONE_CHILD_CLEARTID */
    unsigned long flags = 0x003d0f00;
    char *stack_top = (char *)stack_base + stack_size;
    t->tid = 1;

    long ret = __clone(__fakecc_thread_trampoline, stack_top, flags, (void *)t, (void *)tcb, (void *)&t->tid);
    if (ret < 0) {
        __syscall(11, (long)stack_base, (long)stack_size);
        __syscall(11, (long)tls_base, (long)tls_alloc);
        return (int)(-ret);
    }

    *thread = (pthread_t)t;
    return 0;
}

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg) {
    return pthread_create((pthread_t *)thread, 0, start_routine, arg);
}

int pthread_join(pthread_t thread, void **retval) {
    struct __fakecc_thread *t = (struct __fakecc_thread *)thread;
    if (!t) return -1;

    while (t->tid != 0) {
        __syscall(202, (long)&t->tid, 0 /* FUTEX_WAIT */, (long)t->tid, 0, 0, 0);
    }

    if (retval) {
        *retval = t->retval;
    }

    t->joined = 1;
    t->magic = 0;
    void *stack_base = t->stack_base;
    size_t stack_size = t->stack_size;
    void *tls_base = t->tls_base;
    size_t tls_size = t->tls_size;

    if (stack_base && stack_size) {
        __syscall(11, (long)stack_base, (long)stack_size);
    }
    if (tls_base && tls_size) {
        __syscall(11, (long)tls_base, (long)tls_size);
    }
    return 0;
}

int thread_join(thread_t thread, void **retval) {
    return pthread_join((pthread_t)thread, retval);
}
