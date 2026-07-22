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
    /* ReturnStmt.value is now Expr* */
    T_ASSERT(tu.functions.data[0].body.value != NULL);
    T_ASSERT_EQ_INT((int)tu.functions.data[0].body.value->kind, (int)EX_INT_LIT);
    T_ASSERT_EQ_INT(tu.functions.data[0].body.value->u.int_val, 42);
    tu_free(&tu);
}

static void test_return_zero(void) {
    TranslationUnit tu = lex_parse("package main; int main() { return 0; }");
    T_ASSERT_EQ_INT(tu.functions.data[0].body.value->u.int_val, 0);
    tu_free(&tu);
}

static void test_return_255(void) {
    TranslationUnit tu = lex_parse("package main; int main() { return 255; }");
    T_ASSERT_EQ_INT(tu.functions.data[0].body.value->u.int_val, 255);
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

/* ---- Slice 2: expression AST tests ---- */

static void test_add_expr(void) {
    /* return 1+2; → BOP_ADD(INT_LIT(1), INT_LIT(2)) */
    TranslationUnit tu = lex_parse("package main; int main() { return 1+2; }");
    Expr *e = tu.functions.data[0].body.value;
    T_ASSERT_EQ_INT((int)e->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.op, (int)BOP_ADD);
    T_ASSERT_EQ_INT((int)e->u.bin.l->kind, (int)EX_INT_LIT);
    T_ASSERT_EQ_INT(e->u.bin.l->u.int_val, 1);
    T_ASSERT_EQ_INT((int)e->u.bin.r->kind, (int)EX_INT_LIT);
    T_ASSERT_EQ_INT(e->u.bin.r->u.int_val, 2);
    tu_free(&tu);
}

static void test_mul_priority(void) {
    /* return 1+2*3; → BOP_ADD(1, BOP_MUL(2, 3)) */
    TranslationUnit tu = lex_parse("package main; int main() { return 1+2*3; }");
    Expr *e = tu.functions.data[0].body.value;
    T_ASSERT_EQ_INT((int)e->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.op, (int)BOP_ADD);
    T_ASSERT_EQ_INT((int)e->u.bin.r->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.r->u.bin.op, (int)BOP_MUL);
    tu_free(&tu);
}

static void test_paren_expr(void) {
    /* return (1+2)*3; → BOP_MUL(BOP_ADD(1,2), 3) */
    TranslationUnit tu = lex_parse("package main; int main() { return (1+2)*3; }");
    Expr *e = tu.functions.data[0].body.value;
    T_ASSERT_EQ_INT((int)e->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.op, (int)BOP_MUL);
    T_ASSERT_EQ_INT((int)e->u.bin.l->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.l->u.bin.op, (int)BOP_ADD);
    tu_free(&tu);
}

static void test_left_assoc_sub(void) {
    /* return 1-2-3; → BOP_SUB(BOP_SUB(1,2), 3) */
    TranslationUnit tu = lex_parse("package main; int main() { return 1-2-3; }");
    Expr *e = tu.functions.data[0].body.value;
    T_ASSERT_EQ_INT((int)e->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.op, (int)BOP_SUB);
    T_ASSERT_EQ_INT((int)e->u.bin.l->kind, (int)EX_BINOP);
    T_ASSERT_EQ_INT((int)e->u.bin.l->u.bin.op, (int)BOP_SUB);
    tu_free(&tu);
}

static void test_unary_neg(void) {
    /* return -5; → EX_UNARY(UOP_NEG, INT_LIT(5)) */
    TranslationUnit tu = lex_parse("package main; int main() { return -5; }");
    Expr *e = tu.functions.data[0].body.value;
    T_ASSERT_EQ_INT((int)e->kind, (int)EX_UNARY);
    T_ASSERT_EQ_INT((int)e->u.un.op, (int)UOP_NEG);
    T_ASSERT_EQ_INT((int)e->u.un.operand->kind, (int)EX_INT_LIT);
    T_ASSERT_EQ_INT(e->u.un.operand->u.int_val, 5);
    tu_free(&tu);
}

static void test_unary_pos(void) {
    /* return +5; → EX_UNARY(UOP_POS, INT_LIT(5)) */
    TranslationUnit tu = lex_parse("package main; int main() { return +5; }");
    Expr *e = tu.functions.data[0].body.value;
    T_ASSERT_EQ_INT((int)e->kind, (int)EX_UNARY);
    T_ASSERT_EQ_INT((int)e->u.un.op, (int)UOP_POS);
    T_ASSERT_EQ_INT(e->u.un.operand->u.int_val, 5);
    tu_free(&tu);
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
    test_add_expr();
    test_mul_priority();
    test_paren_expr();
    test_left_assoc_sub();
    test_unary_neg();
    test_unary_pos();
    return t_finalize();
}
