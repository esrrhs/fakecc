#include "fakecc/ast.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

static SourceLoc loc(void) {
    SourceLoc l;
    l.file = "t.c";
    l.line = 1;
    l.col = 1;
    return l;
}

static void test_align16_and_empty(void) {
    Type ld = type_make_float(16);
    T_ASSERT(type_needs_stack_align16(ld));
    type_free(&ld);
    Type i128 = type_make_int(16, 0);
    T_ASSERT(type_needs_stack_align16(i128));
    Type cld = type_make_struct("__complex_ldouble", 32);
    T_ASSERT(type_is_complex_ldouble(cld));
    T_ASSERT(type_needs_stack_align16(cld));
    type_free(&cld);
    Type small = type_make_int(4, 0);
    T_ASSERT(!type_needs_stack_align16(small));
    Type empty = type_make_struct("E", 0);
    T_ASSERT(type_is_empty_struct(empty));
    type_free(&empty);
}

static void test_typedef_and_func_equal(void) {
    Type i = type_make_int(4, 0);
    Type u = type_make_int(4, 1);
    T_ASSERT(type_same_typedef(i, i));
    T_ASSERT(!type_same_typedef(i, u));
    T_ASSERT(type_same_typedef(type_make_void(), type_make_void()));
    T_ASSERT(type_same_typedef(type_make_float(8), type_make_float(8)));
    T_ASSERT(!type_same_typedef(type_make_float(4), type_make_float(8)));

    Type pi = type_make_ptr(i);
    Type pi2 = type_make_ptr(i);
    T_ASSERT(type_same_typedef(pi, pi2));
    type_free(&pi);
    type_free(&pi2);

    Type arr = type_make_array(i, 4);
    Type arr2 = type_make_array(i, 4);
    Type arr3 = type_make_array(i, 8);
    T_ASSERT(type_same_typedef(arr, arr2));
    T_ASSERT(!type_same_typedef(arr, arr3));
    type_free(&arr);
    type_free(&arr2);
    type_free(&arr3);

    Type *ps[1];
    Type p0 = type_make_int(4, 0);
    ps[0] = &p0;
    Type f1 = type_make_func(i, ps, 1);
    Type f2 = type_make_func(i, ps, 1);
    Type f3 = type_make_func(u, ps, 1);
    T_ASSERT(type_funcs_equal(f1, f2));
    T_ASSERT(!type_funcs_equal(f1, f3));
    T_ASSERT(!type_funcs_equal(i, f1));
    T_ASSERT(type_same_typedef(f1, f2));
    Type fv = type_make_func_var(i, ps, 1, 1);
    T_ASSERT(!type_same_typedef(f1, fv));
    type_free(&f1);
    type_free(&f2);
    type_free(&f3);
    type_free(&fv);
}

static void test_sysv_classify(void) {
    SysVRegClass cls[2] = {0, 0};
    Type vf = type_make_vector(type_make_float(4), 16);
    T_ASSERT_EQ_INT(sysv_classify_agg(vf, cls), 1);
    T_ASSERT_EQ_INT((int)cls[0], (int)SYSV_CLS_SSE);
    type_free(&vf);

    Type v8 = type_make_vector(type_make_float(4), 8);
    T_ASSERT_EQ_INT(sysv_classify_agg(v8, cls), 1);
    type_free(&v8);

    Type v4 = type_make_vector(type_make_int(4, 0), 4);
    T_ASSERT(sysv_classify_agg(v4, cls) >= 1);
    type_free(&v4);

    Type v64 = type_make_vector(type_make_float(4), 64);
    T_ASSERT_EQ_INT(sysv_classify_agg(v64, cls), 0); /* MEMORY without AVX-512 */
    type_free(&v64);

    Type s = type_make_struct("S8", 8);
    T_ASSERT_EQ_INT(sysv_classify_agg(s, cls), 1);
    type_free(&s);
    Type s16 = type_make_struct("S16", 16);
    T_ASSERT_EQ_INT(sysv_classify_agg(s16, cls), 2);
    type_free(&s16);
    Type big = type_make_struct("Big", 32);
    T_ASSERT_EQ_INT(sysv_classify_agg(big, cls), 0);
    type_free(&big);
}

static void test_fold_int128(void) {
    unsigned long long lo = 0, hi = 0;
    Expr *e = expr_new_int(42, loc());
    T_ASSERT(fold_const_int128(e, &lo, &hi));
    T_ASSERT_EQ_INT((int)lo, 42);
    expr_free(e);

    e = expr_new_var("__CHAR_BIT__", loc());
    T_ASSERT(fold_const_int128(e, &lo, &hi));
    T_ASSERT_EQ_INT((int)lo, 8);
    expr_free(e);

    e = expr_new_var("__INT_MAX__", loc());
    T_ASSERT(fold_const_int128(e, &lo, &hi));
    T_ASSERT_EQ_INT((int)lo, 0x7fffffff);
    expr_free(e);

    Expr *inner = expr_new_int(3, loc());
    e = expr_new_cast(type_make_int(1, 0), inner, loc());
    T_ASSERT(fold_const_int128(e, &lo, &hi));
    expr_free(e);

    inner = expr_new_int(-1, loc());
    e = expr_new_cast(type_make_bool(), inner, loc());
    T_ASSERT(fold_const_int128(e, &lo, &hi));
    T_ASSERT_EQ_INT((int)lo, 1);
    expr_free(e);

    e = expr_new_unary(UOP_POS, expr_new_int(9, loc()), loc());
    long long v = 0;
    T_ASSERT(fold_const_int(e, &v));
    T_ASSERT_EQ_INT((int)v, 9);
    expr_free(e);

    T_ASSERT(!fold_const_int128(NULL, &lo, &hi));
}

static void test_stmt_and_expr_clone(void) {
    TokenArray arr;
    token_array_init(&arr);
    lex("package main;\n"
        "int main() {\n"
        "  int x; x = 1;\n"
        "  if (x) { x = 2; } else { x = 3; }\n"
        "  while (x) { x = x - 1; }\n"
        "  do { x = x + 1; } while (x < 2);\n"
        "  for (int i = 0; i < 1; i = i + 1) { x = i; }\n"
        "  switch (x) { case 1: x = 4; break; default: x = 5; }\n"
        "  { int y; y = x; }\n"
        "  return x;\n"
        "}\n",
        "t.c", &arr);
    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    T_ASSERT(tu.functions.len >= 1);
    StmtArray *body = &tu.functions.data[0].body;
    size_t i;
    for (i = 0; i < body->len; i++) {
        Stmt c = stmt_clone(&body->data[i]);
        T_ASSERT_EQ_INT((int)c.kind, (int)body->data[i].kind);
        stmt_free(&c);
    }
    Stmt none = stmt_clone(NULL);
    T_ASSERT_EQ_INT((int)none.kind, 0);
    tu_free(&tu);
    token_array_free(&arr);
}

static void test_more_expr_helpers(void) {
    SourceLoc l = loc();
    Expr *e = expr_new_int_typed(5, 8, 1, l);
    T_ASSERT_EQ_INT(e->type.width, 8);
    expr_free(e);
    e = expr_new_var_qual("pkg", "name", l);
    T_ASSERT(e->u.var.pkg != NULL);
    expr_free(e);

    Expr *call = expr_new_call(expr_new_var("f", l), l);
    expr_call_set_callee(call, expr_new_var("g", l));
    Expr *c2 = expr_clone(call);
    T_ASSERT(c2 != NULL);
    expr_free(call);
    expr_free(c2);

    Expr *as = expr_new_assign(expr_new_var("x", l), expr_new_int(1, l), l);
    Expr *idx = expr_new_index(expr_new_var("a", l), expr_new_int(0, l), l);
    Expr *mem = expr_new_member(expr_new_var("s", l), "x", l);
    Expr *sz = expr_new_sizeof_expr(expr_new_unary(UOP_POS, expr_new_var("n", l), l), l);
    Expr *ca = expr_new_compound_assign(expr_new_var("x", l), expr_new_int(1, l), BOP_ADD, l);
    Expr *cm = expr_new_comma(expr_new_int(1, l), expr_new_int(2, l), l);
    Expr *tern = expr_new_ternary(expr_new_int(1, l), expr_new_int(2, l), expr_new_int(3, l), l);
    Expr *clones[] = {as, idx, mem, sz, ca, cm, tern};
    int i;
    for (i = 0; i < 7; i++) {
        Expr *x = expr_clone(clones[i]);
        T_ASSERT(x != NULL);
        expr_free(x);
        expr_free(clones[i]);
    }
}

int main(void) {
    test_align16_and_empty();
    test_typedef_and_func_equal();
    test_sysv_classify();
    test_fold_int128();
    test_stmt_and_expr_clone();
    test_more_expr_helpers();
    return t_finalize();
}
