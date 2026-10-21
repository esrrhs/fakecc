/* Windows thread layer — native Win64 ABI, KERNEL32 threading, and PE TLS.
 * Selected by the _windows suffix in the package loader. */
package runtime;

typedef unsigned long pthread_t;
typedef unsigned long thread_t;

typedef struct pthread_attr_t {
    int __detachstate;
    size_t __stacksize;
} pthread_attr_t;

static const unsigned long FAKECC_THREAD_MAGIC = 0x54485244;

struct __fakecc_thread {
    unsigned long magic;
    void *(*start_routine)(void *);
    void *arg;
    void *retval;
    void *handle;
    unsigned long tid;
    volatile int done;
    volatile int joined;
};

/* KERNEL32 function prototypes imported via PE IAT */
void *CreateThread(void *sa, unsigned long stack, void *start, void *param, unsigned long flags, unsigned long *id);
unsigned long WaitForSingleObject(void *handle, unsigned long ms);
int CloseHandle(void *handle);
void ExitThread(unsigned long exit_code);
unsigned long GetCurrentThreadId(void);
void *VirtualAlloc(void *addr, unsigned long size, unsigned long alloc_type, unsigned long protect);
int VirtualFree(void *addr, unsigned long size, unsigned long free_type);

static struct __fakecc_thread *__fakecc_threads[4096];
static struct __fakecc_thread __main_thread;
static __thread struct __fakecc_thread *__fakecc_current_thread;

static int __fakecc_track_thread(struct __fakecc_thread *t) {
    int i;
    for (i = 0; i < 4096; i = i + 1) {
        if (__fakecc_threads[i] == 0) {
            __fakecc_threads[i] = t;
            return 0;
        }
    }
    return -1;
}

static int __fakecc_untrack_thread(struct __fakecc_thread *t) {
    int i;
    for (i = 0; i < 4096; i = i + 1) {
        if (__fakecc_threads[i] == t) {
            __fakecc_threads[i] = 0;
            return 0;
        }
    }
    return -1;
}

static int __fakecc_is_tracked_thread(struct __fakecc_thread *t) {
    int i;
    if (t == 0) return 0;
    for (i = 0; i < 4096; i = i + 1) {
        if (__fakecc_threads[i] == t) {
            return 1;
        }
    }
    return 0;
}

/* Win64 thread trampoline called by KERNEL32 CreateThread */
static unsigned long __fakecc_win_thread_trampoline(void *arg) {
    struct __fakecc_thread *t = (struct __fakecc_thread *)arg;
    __fakecc_current_thread = t;
    void *res = t->start_routine(t->arg);
    t->retval = res;
    t->done = 1;
    ExitThread(0);
    return 0;
}

pthread_t pthread_self(void) {
    if (__fakecc_current_thread) {
        return (pthread_t)__fakecc_current_thread;
    }
    __main_thread.magic = FAKECC_THREAD_MAGIC;
    __main_thread.tid = GetCurrentThreadId();
    __main_thread.handle = 0;
    __main_thread.done = 0;
    __main_thread.joined = 0;
    __main_thread.retval = 0;
    __fakecc_current_thread = &__main_thread;
    return (pthread_t)&__main_thread;
}

thread_t thread_self(void) {
    return (thread_t)pthread_self();
}

void pthread_exit(void *retval) {
    struct __fakecc_thread *t = (struct __fakecc_thread *)pthread_self();
    if (t && t != &__main_thread) {
        t->retval = retval;
        t->done = 1;
        ExitThread(0);
    }
    ExitThread(0);
}

void thread_exit(void *retval) {
    pthread_exit(retval);
}

int pthread_create(pthread_t *thread, const void *attr,
                   void *(*start_routine)(void *), void *arg) {
    if (thread == 0 || start_routine == 0) return -1;

    size_t stack_size = 0;
    if (attr != 0) {
        stack_size = ((const pthread_attr_t *)attr)->__stacksize;
    }

    struct __fakecc_thread *t = (struct __fakecc_thread *)VirtualAlloc(0, 4096, 0x3000, 4);
    if (t == 0) return -1;

    t->magic = FAKECC_THREAD_MAGIC;
    t->start_routine = start_routine;
    t->arg = arg;
    t->retval = 0;
    t->handle = 0;
    t->tid = 0;
    t->done = 0;
    t->joined = 0;

    if (__fakecc_track_thread(t) != 0) {
        VirtualFree((void *)t, 0, 0x8000);
        return -1;
    }

    void *h = CreateThread(0, (unsigned long)stack_size, (void *)__fakecc_win_thread_trampoline, (void *)t, 0, &t->tid);
    if (h == 0) {
        __fakecc_untrack_thread(t);
        VirtualFree((void *)t, 0, 0x8000);
        return -1;
    }
    t->handle = h;
    *thread = (pthread_t)t;
    return 0;
}

int thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg) {
    return pthread_create((pthread_t *)thread, 0, start_routine, arg);
}

int pthread_join(pthread_t thread, void **retval) {
    if (thread == 0) return -1;

    pthread_t self = pthread_self();
    if (thread == self) return -1;
    if (thread == (pthread_t)&__main_thread) return -1;

    struct __fakecc_thread *t = (struct __fakecc_thread *)thread;
    if (!__fakecc_is_tracked_thread(t)) {
        return -1;
    }
    if (t->magic != FAKECC_THREAD_MAGIC) {
        return -1;
    }
    if (t->joined) {
        return -1;
    }
    t->joined = 1;

    if (t->handle) {
        WaitForSingleObject(t->handle, 0xFFFFFFFF);
        CloseHandle(t->handle);
        t->handle = 0;
    }

    if (retval != 0) {
        *retval = t->retval;
    }

    __fakecc_untrack_thread(t);
    t->magic = 0;
    VirtualFree((void *)t, 0, 0x8000);
    return 0;
}

int thread_join(thread_t thread, void **retval) {
    return pthread_join((pthread_t)thread, retval);
}
