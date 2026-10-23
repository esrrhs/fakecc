#include "fakecc/ast.h"
#include "fakecc/common.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include "win_isolate.h"
#endif

/* ---- helper: lex + parse + sema_check; returns 1 on error ---- */
static int lex_parse_sema_errors(const char *src) {
    fakecc_clear_error();
    TokenArray arr;
    token_array_init(&arr);
    if (lex(src, "test.c", &arr) != FAKECC_OK || fakecc_had_error()) {
        token_array_free(&arr);
        return 1;
    }

    TranslationUnit tu;
    tu_init(&tu);
    if (parse(&arr, &tu) != FAKECC_OK || fakecc_had_error()) {
        token_array_free(&arr);
        return 1;
    }
    token_array_free(&arr);

    int rc = sema_check(&tu, 1);
    int err = rc != FAKECC_OK || fakecc_had_error() || sema_has_errors();
    if (!err) tu_free(&tu);
    return err;
}


/* ---- helper: run in fork, expect non-zero exit ---- */
static int fork_dies(const char *src) {
    int pid = fork();
    if (pid == 0)
        _exit(lex_parse_sema_errors(src) ? 1 : 0);
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) != 0;
}

/* ---- helper: run in fork, expect success (exit 0) ---- */
static int fork_ok(const char *src) {
    int pid = fork();
    if (pid == 0)
        _exit(lex_parse_sema_errors(src) ? 1 : 0);
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
    /* Non-void `main` must contain a return (or a call / infinite loop that
     * does not fall through). Falling off the end is an error. */
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

static void test_const_assign(void) {
    T_ASSERT(fork_dies(
        "package main; int main() { const int x = 1; x = 2; return x; }"));
}

static void test_void_var(void) {
    T_ASSERT(fork_dies("package main; int main() { void x; return 0; }"));
}

static void test_call_arity(void) {
    T_ASSERT(fork_dies(
        "package main; int f(int a, int b) { return a + b; }"
        "int main() { return f(1); }"));
}

static void test_deref_int(void) {
    T_ASSERT(fork_dies(
        "package main; int main() { int x; return *x; }"));
}

static void test_break_toplevel(void) {
    T_ASSERT(fork_dies("package main; int main() { break; return 0; }"));
}

static void test_goto_undef(void) {
    T_ASSERT(fork_dies(
        "package main; int main() { goto missing; return 0; }"));
}

static void test_valid_struct(void) {
    T_ASSERT(fork_ok(
        "package main; struct S { int a; int b; };"
        "int main() { struct S s; s.a = 1; s.b = 2; return s.a + s.b; }"));
}

static void test_valid_bitfield(void) {
    T_ASSERT(fork_ok(
        "package main; struct F { unsigned a : 3; unsigned b : 5; };"
        "int main() { struct F f; f.a = 1; f.b = 2; return f.a + f.b; }"));
}

static void test_int128_const_ops(void) {
    /* Test int128 << 0 doesn't corrupt high bits and !0 folds correctly */
    T_ASSERT(fork_ok(
        "package main; "
        "int main() { "
        "  static __int128 a = (__int128)0x100000000ULL << 0; "
        "  static __int128 b = !(__int128)0; "
        "  static __int128 c = !(__int128)5; "
        "  static __int128 d = __CHAR_BIT__; "
        "  return 0; "
        "}"));
}

/* Teardown after a semantic error walks AST nodes that normalize_init_list
 * had already folded into its dense output and freed; list->u.init_list.
 * elements still named them, so expr_free() hit freed memory.  Only a
 * sanitizer notices -- which is exactly how the fuzzer found it -- and the
 * unit legs run under ASan/UBSan on both platforms, so this is the guard. */
static void test_init_list_error_teardown(void) {
    const char *src =
        "package main;\n"
        "struct T { int a; int b; int c; };\n"
        "struct U { struct T t; };\n"
        "int main(void) {\n"
        "  struct U u = { .t.a = 1, .t.c = 3, .tnb = 2 };\n"
        "  return 0;\n"
        "}\n";
    fakecc_clear_error();
    TokenArray arr;
    token_array_init(&arr);
    T_ASSERT_EQ_INT(lex(src, "test.c", &arr), FAKECC_OK);
    TranslationUnit tu;
    tu_init(&tu);
    T_ASSERT_EQ_INT(parse(&arr, &tu), FAKECC_OK);
    token_array_free(&arr);
    sema_check(&tu, 1);
    T_ASSERT(fakecc_had_error() || sema_has_errors());
    tu_free(&tu);
    fakecc_clear_error();
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
    test_const_assign();
    test_void_var();
    test_call_arity();
    test_deref_int();
    test_break_toplevel();
    test_goto_undef();
    test_valid_struct();
    test_valid_bitfield();
    test_int128_const_ops();
    test_init_list_error_teardown();
    return t_finalize();
}
