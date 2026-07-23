#include "fakecc/ast.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ---- helper: lex + parse + sema_check ---- */
static void lex_parse_sema(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);

    sema_check(&tu);

    tu_free(&tu);
    token_array_free(&arr);
}

/* ---- helper: run in fork, expect non-zero exit ---- */
static int fork_dies(const char *src) {
    int pid = fork();
    if (pid == 0) {
        lex_parse_sema(src);
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) != 0;
}

/* ---- helper: run in fork, expect success (exit 0) ---- */
static int fork_ok(const char *src) {
    int pid = fork();
    if (pid == 0) {
        lex_parse_sema(src);
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

/* ---- tests ---- */

static void test_undeclared_var(void) {
    T_ASSERT(fork_dies("package main; int main() { return x; }"));
}

static void test_redecl(void) {
    T_ASSERT(fork_dies("package main; int main() { int x; int x; return 0; }"));
}

static void test_assign_undeclared(void) {
    T_ASSERT(fork_dies("package main; int main() { x = 5; return 0; }"));
}

static void test_assign_nonlvalue(void) {
    T_ASSERT(fork_dies("package main; int main() { 5 = 3; return 0; }"));
}

static void test_no_return(void) {
    T_ASSERT(fork_dies("package main; int main() { int x; x = 5; }"));
}

static void test_use_before_decl(void) {
    T_ASSERT(fork_dies("package main; int main() { x = 5; int x; return x; }"));
}

static void test_valid_var(void) {
    T_ASSERT(fork_ok("package main; int main() { int x; x = 1; return x; }"));
}

static void test_valid_return_expr(void) {
    T_ASSERT(fork_ok("package main; int main() { return 42; }"));
}

/* ---- main ---- */

int main(void) {
    test_undeclared_var();
    test_redecl();
    test_assign_undeclared();
    test_assign_nonlvalue();
    test_no_return();
    test_use_before_decl();
    test_valid_var();
    test_valid_return_expr();
    return t_finalize();
}
