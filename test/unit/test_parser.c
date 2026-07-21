#include "fakecc/ast.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ---- helper: lex + parse ---- */
static TranslationUnit lex_parse(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);

    token_array_free(&arr);
    return tu;
}

/* ---- tests ---- */

static void test_valid_program(void) {
    TranslationUnit tu = lex_parse("package main; int main() { return 42; }");
    T_ASSERT_STR_EQ(tu.package.name, "main");
    T_ASSERT_EQ_INT((int)tu.functions.len, 1);
    T_ASSERT_STR_EQ(tu.functions.data[0].name, "main");
    T_ASSERT_EQ_INT(tu.functions.data[0].body.value, 42);
    tu_free(&tu);
}

static void test_return_zero(void) {
    TranslationUnit tu = lex_parse("package main; int main() { return 0; }");
    T_ASSERT_EQ_INT(tu.functions.data[0].body.value, 0);
    tu_free(&tu);
}

static void test_return_255(void) {
    TranslationUnit tu = lex_parse("package main; int main() { return 255; }");
    T_ASSERT_EQ_INT(tu.functions.data[0].body.value, 255);
    tu_free(&tu);
}

static void test_different_package_name(void) {
    TranslationUnit tu = lex_parse("package foo; int main() { return 1; }");
    T_ASSERT_STR_EQ(tu.package.name, "foo");
    tu_free(&tu);
}

/* Error-path tests use fork to catch die_at exit.
 * Slice 1 spec says error paths only need e2e coverage,
 * but we add a few fork-based unit tests for convenience. */

static void test_missing_package_dies(void) {
    /* "int main() { return 0; }" — no package decl */
    int pid = fork();
    if (pid == 0) {
        lex_parse("int main() { return 0; }");
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

static void test_import_dies(void) {
    int pid = fork();
    if (pid == 0) {
        lex_parse("package main; import \"foo\"; int main() { return 0; }");
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

static void test_missing_semicolon_dies(void) {
    int pid = fork();
    if (pid == 0) {
        lex_parse("package main int main() { return 0; }");
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

static void test_return_without_int_dies(void) {
    int pid = fork();
    if (pid == 0) {
        lex_parse("package main; int main() { return ; }");
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

/* ---- main ---- */

int main(void) {
    test_valid_program();
    test_return_zero();
    test_return_255();
    test_different_package_name();
    test_missing_package_dies();
    test_import_dies();
    test_missing_semicolon_dies();
    test_return_without_int_dies();
    return t_finalize();
}
