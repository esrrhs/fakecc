#include "fakecc/common.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

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

static void test_die_at_records_error(void) {
    fakecc_clear_error();
    T_ASSERT(!fakecc_had_error());
    int rc = die_at("t.c", 1, 2, "boom %d", 7);
    T_ASSERT_EQ_INT(rc, FAKECC_ERR);
    T_ASSERT(fakecc_had_error());
    T_ASSERT_EQ_INT(fakecc_error_code(), FAKECC_ERR);
    T_ASSERT_STR_EQ(fakecc_error_message(), "boom 7");
    SourceLoc loc = fakecc_error_loc();
    T_ASSERT_STR_EQ(loc.file, "t.c");
    T_ASSERT_EQ_INT(loc.line, 1);
    T_ASSERT_EQ_INT(loc.col, 2);
    /* Second call keeps the first message and still returns FAKECC_ERR. */
    rc = die_at("other.c", 9, 9, "ignored");
    T_ASSERT_EQ_INT(rc, FAKECC_ERR);
    T_ASSERT_STR_EQ(fakecc_error_message(), "boom 7");
    fakecc_clear_error();
    T_ASSERT(!fakecc_had_error());
}

int main(void) {
    test_buffer_append_and_fmt();
    test_xmalloc_zero_and_strdup();
    test_die_at_records_error();
    return t_finalize();
}
