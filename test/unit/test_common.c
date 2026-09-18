#include "fakecc/common.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

static void test_buffer_append_and_fmt(void) {
    Buffer b;
    buffer_init(&b);
    buffer_append(&b, "abc", 0);
    T_ASSERT_EQ_INT((int)b.len, 0);
    buffer_append(&b, "hello", 5);
    T_ASSERT_EQ_INT((int)b.len, 5);
    buffer_appendf(&b, " %s %d", "world", 42);
    T_ASSERT(b.len > 5);
    T_ASSERT(b.data != NULL);
    T_ASSERT(memcmp(b.data, "hello", 5) == 0);
    buffer_free(&b);
}

static void test_xmalloc_zero_and_strdup(void) {
    void *p = xmalloc(0);
    T_ASSERT(p != NULL);
    free(p);
    char *s = xstrdup("xyz");
    T_ASSERT_STR_EQ(s, "xyz");
    char *g = xrealloc(s, 16);
    T_ASSERT(g != NULL);
    T_ASSERT_STR_EQ(g, "xyz");
    free(g);
}

static void test_die_at_exits(void) {
    int pid = fork();
    if (pid == 0) {
        die_at("t.c", 1, 2, "boom %d", 7);
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

int main(void) {
    test_buffer_append_and_fmt();
    test_xmalloc_zero_and_strdup();
    test_die_at_exits();
    return t_finalize();
}
