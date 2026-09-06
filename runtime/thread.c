/* POSIX-compatible and lightweight thread runtime — FakeCC dialect. */
package runtime;

typedef unsigned long pthread_t;
typedef unsigned long thread_t;

typedef struct {
    int __detachstate;
    size_t __stacksize;
} pthread_attr_t;

struct __fakecc_thread {
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

static __thread struct __fakecc_thread *__cur_thread;
static struct __fakecc_thread __main_thread;

static void *__fakecc_thread_trampoline(void *arg) {
    struct __fakecc_thread *t = (struct __fakecc_thread *)arg;
    __cur_thread = t;
    void *res = t->start_routine(t->arg);
    t->retval = res;
    t->done = 1;
    __syscall(202, (long)&t->done, 1 /* FUTEX_WAKE */, 1, 0, 0, 0);
    return res;
}

void pthread_exit(void *retval) {
    if (__cur_thread) {
        __cur_thread->retval = retval;
        __cur_thread->done = 1;
        __syscall(202, (long)&__cur_thread->done, 1 /* FUTEX_WAKE */, 1, 0, 0, 0);
    }
    __syscall(60, 0);
}

void thread_exit(void *retval) {
    pthread_exit(retval);
}

pthread_t pthread_self(void) {
    if (!__cur_thread) {
        __cur_thread = &__main_thread;
    }
    return (pthread_t)__cur_thread;
}

thread_t thread_self(void) {
    return (thread_t)pthread_self();
}

int pthread_create(pthread_t *thread, const void *attr, void *(*start_routine)(void *), void *arg) {
    if (!thread || !start_routine) return -1;

    size_t stack_size = 1024 * 1024; /* 1MB stack */
    void *stack_base = (void *)__syscall(9, 0, (long)stack_size, 3 /* PROT_READ|PROT_WRITE */, 0x22 /* MAP_PRIVATE|MAP_ANONYMOUS */, -1, 0);
    if ((long)stack_base < 0) return -1;

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

    struct __fakecc_thread *t = (struct __fakecc_thread *)malloc(sizeof(struct __fakecc_thread));
    if (!t) {
        __syscall(11, (long)stack_base, (long)stack_size);
        __syscall(11, (long)tls_base, (long)tls_alloc);
        return -1;
    }
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
        free(t);
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
    if (t->stack_base && t->stack_size) {
        __syscall(11, (long)t->stack_base, (long)t->stack_size);
    }
    if (t->tls_base && t->tls_size) {
        __syscall(11, (long)t->tls_base, (long)t->tls_size);
    }
    free(t);
    return 0;
}

int thread_join(thread_t thread, void **retval) {
    return pthread_join((pthread_t)thread, retval);
}
