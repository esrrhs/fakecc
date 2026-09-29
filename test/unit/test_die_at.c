#include "fakecc/ast.h"
#include "fakecc/common.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

typedef enum { ST_LEX = 1, ST_PARSE, ST_SEMA, ST_IR } Stage;

typedef struct {
    Stage stage;
    const char *name;
    const char *src;
} DieCase;

/* Run one pipeline stage; return 1 if that stage reported an error. */
static int stage_errors(Stage st, const char *src) {
    fakecc_clear_error();
    TokenArray arr;
    token_array_init(&arr);
    int rc = lex(src, "t.c", &arr);
    if (st == ST_LEX) {
        token_array_free(&arr);
        return rc != FAKECC_OK || fakecc_had_error();
    }
    if (rc != FAKECC_OK) {
        token_array_free(&arr);
        return 1;
    }
    TranslationUnit tu;
    tu_init(&tu);
    rc = parse(&arr, &tu);
    token_array_free(&arr);
    if (st == ST_PARSE) {
        int err = rc != FAKECC_OK || fakecc_had_error();
        /* Skip tu_free on error — partial ASTs may not be safe to free. */
        if (!err) tu_free(&tu);
        return err;
    }
    if (rc != FAKECC_OK) {
        return 1;
    }
    rc = sema_check(&tu, 1);
    if (st == ST_SEMA) {
        int err = rc != FAKECC_OK || fakecc_had_error() || sema_has_errors();
        if (!err) tu_free(&tu);
        return err;
    }
    if (rc != FAKECC_OK) {
        return 1;
    }
    IRModule ir;
    ir_module_init(&ir);
    rc = ir_generate(&tu, &ir, 0);
    int err = rc != FAKECC_OK || fakecc_had_error();
    if (!err) {
        ir_module_free(&ir);
        tu_free(&tu);
    }
    return err;
}

/* Fork so a crash/abort on a partial AST cannot take down the suite.
 * The child exits non-zero iff the stage reported an error. */
static int dies(Stage st, const char *src) {
    int pid = fork();
    if (pid == 0) {
        int nulfd = open("/dev/null", O_WRONLY);
        if (nulfd >= 0) {
            dup2(nulfd, STDERR_FILENO);
            close(nulfd);
        }
        int err = stage_errors(st, src);
        _exit(err ? 1 : 0);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) != 0;
}

static const char *too_many_params(void) {
    static char *buf;
    if (buf)
        return buf;
    size_t cap = 32u * 1026u + 80u;
    buf = malloc(cap);
    if (!buf)
        exit(1);
    char *p = buf;
    p += sprintf(p, "package main; int f(");
    int i;
    for (i = 0; i < 1025; i++) {
        if (i)
            p += sprintf(p, ", ");
        p += sprintf(p, "int a%d", i);
    }
    sprintf(p, ") { return 0; } int main() { return 0; }");
    return buf;
}

static const DieCase k_cases[] = {
    /* ---- lexer ---- */
    {ST_LEX, "preprocessor", "package main;\n# define X 1\nint main(){return 0;}"},
    {ST_LEX, "unterminated_char_nl", "package main; int main() { return '\n; }"},
    {ST_LEX, "hex_escape_no_digits", "package main; int main() { return '\\x'; }"},
    {ST_LEX, "unterminated_char_eof", "package main; int main() { char c = '"},
    {ST_LEX, "hex_no_digits", "package main; int main() { return 0x; }"},
    {ST_LEX, "hex_float_exp", "package main; int main() { return 0x1.p; }"},
    {ST_LEX, "exp_no_digits", "package main; int main() { return 1e; }"},
    {ST_LEX, "dot_exp_no_digits", "package main; int main() { return .5e; }"},
    {ST_LEX, "unexpected_at", "package main; int main() { return @; }"},
    {ST_LEX, "missing_char_quote", "package main; int main() { return 'a; }"},

    /* ---- parser ---- */
    {ST_PARSE, "struct_tag", "package main; struct 1 { int x; }; int main(){return 0;}"},
    {ST_PARSE, "struct_redef", "package main; struct S { int x; }; struct S { int y; }; int main(){return 0;}"},
    {ST_PARSE, "struct_redef_with_var",
     "package main; struct S { int x; } a; struct S { int y; } b; int main(){return 0;}"},
    {ST_PARSE, "union_tag", "package main; union 1 { int x; }; int main(){return 0;}"},
    {ST_PARSE, "union_redef", "package main; union U { int x; }; union U { int y; }; int main(){return 0;}"},
    {ST_PARSE, "union_redef_with_var",
     "package main; union U { int x; } a; union U { int y; } b; int main(){return 0;}"},
    {ST_PARSE, "enum_tag", "package main; enum 1; int main(){return 0;}"},
    {ST_PARSE, "restrict_nonptr", "package main; int restrict x; int main(){return 0;}"},
    {ST_PARSE, "member_name", "package main; struct S { int 1; }; int main(){return 0;}"},
    {ST_PARSE, "bitfield_width", "package main; int k; struct S { int a : k; }; int main(){return 0;}"},
    {ST_PARSE, "enum_underlying_float", "package main; enum E : float { A }; int main(){return 0;}"},
    {ST_PARSE, "enum_const_name", "package main; enum { 1 }; int main(){return 0;}"},
    {ST_PARSE, "enum_nonconst", "package main; int k; enum { A = k }; int main(){return 0;}"},
    {ST_PARSE, "param_type", "package main; int f(1) { return 0; } int main(){return 0;}"},
    {ST_PARSE, "too_many_ptrs", "package main; int main() { int *********p; return 0; }"},
    {ST_PARSE, "unmatched_paren_expr", "package main; int main() { return (1; }"},
    {ST_PARSE, "unmatched_paren_decl", "package main; int main() { int (*p; return 0; }"},
    {ST_PARSE, "too_many_array_dims",
     "package main; int main() { int a[1][1][1][1][1][1][1][1][1]; return 0; }"},
    {ST_PARSE, "too_many_array_dims_abs",
     "package main; int main() { return (int)sizeof(int[1][1][1][1][1][1][1][1][1]); }"},
    {ST_PARSE, "too_many_array_dims_grouped",
     "package main; int main() { int (a)[1][1][1][1][1][1][1][1][1]; return 0; }"},
    {ST_PARSE, "alignof_no_paren", "package main; int main() { return _Alignof int; }"},
    {ST_PARSE, "offsetof_missing_member",
     "package main; struct S { int x; }; int main() { return __builtin_offsetof(struct S, no); }"},
    {ST_PARSE, "arrow_no_member", "package main; struct S { int x; } *p; int main() { return p->; }"},
    {ST_PARSE, "dot_no_member", "package main; struct S { int x; } s; int main() { s. = 1; return 0; }"},
    {ST_PARSE, "invalid_octal", "package main; int main() { return 08; }"},
    {ST_PARSE, "invalid_suffix", "package main; int main() { return 1uu; }"},
    {ST_PARSE, "designator_no_field",
     "package main; struct S { int x; }; int main() { struct S s = { . = 1 }; return 0; }"},
    {ST_PARSE, "designator_no_index",
     "package main; int main() { int a[2] = { [x] = 1 }; return 0; }"},
    {ST_PARSE, "designator_range_var",
     "package main; int main() { int a[4] = { [0 ... x] = 1 }; return 0; }"},
    {ST_PARSE, "unterminated_init", "package main; int main() { int a[2] = { 1, 2 "},
    {ST_PARSE, "init_missing_comma", "package main; int main() { int a = { 1 2 }; return 0; }"},
    {ST_PARSE, "case_nonconst",
     "package main; int main() { int x; switch (x) { case x: break; } return 0; }"},
    {ST_PARSE, "case_nonfold",
     "package main; int main() { int x; switch (x) { case (x): break; } return 0; }"},
    {ST_PARSE, "case_range_nonfold",
     "package main; int main() { int x; switch (x) { case 1 ... (x): break; } return 0; }"},
    {ST_PARSE, "goto_not_label", "package main; int main() { goto 1; return 0; }"},
    {ST_PARSE, "label_addr_not_name", "package main; int main() { &&1; return 0; }"},
    {ST_PARSE, "typedef_name", "package main; typedef 1 t; int main(){return 0;}"},
    {ST_PARSE, "fn_name", "package main; int 1(void) { return 0; }"},
    {ST_PARSE, "package_name", "package 1; int main(){return 0;}"},
    {ST_PARSE, "expected_type", "package main; const; int main(){return 0;}"},
    {ST_PARSE, "nested_fn",
     "package main; int main() { int nested(int x) { return x; } return 0; }"},
    {ST_PARSE, "stmt_eof", "package main; int main() { return 0;"},
    {ST_PARSE, "expected_expr", "package main; int main() { return ); }"},
    {ST_PARSE, "kr_param_name", "package main; int f(a, 1) { return 0; } int main(){return 0;}"},
    {ST_PARSE, "typedef_redef",
     "package main; typedef int t; typedef float t; int main(){return 0;}"},
    {ST_PARSE, "import_name", "package main; import 1; int main(){return 0;}"},
    {ST_PARSE, "import_no_pkg", "package main; import foo; int main(){return 0;}"},
    {ST_PARSE, "var_name", "package main; int main() { int 1; return 0; }"},
    {ST_PARSE, "file_scope_stmt", "package main; return 0; int main(){return 0;}"},
    {ST_PARSE, "grouped_fn", "package main; int (1)(void) { return 0; }"},
    {ST_PARSE, "kr_decl_name", "package main; int f(a) int; { return a; } int main(){return 0;}"},
    {ST_PARSE, "kr_decl_ptr_no_name",
     "package main; int f(a) int *; { return 0; } int main(){return 0;}"},
    {ST_PARSE, "enum_redef",
     "package main; enum E { A }; enum E { B }; int main(){return 0;}"},
    {ST_PARSE, "fn_ptr_param_type",
     "package main; int (*fp)(1); int main(){return 0;}"},
    {ST_PARSE, "local_struct_redef",
     "package main; int main() { struct S { int x; }; struct S { int y; }; return 0; }"},
    {ST_PARSE, "local_union_redef",
     "package main; int main() { union U { int x; }; union U { int y; }; return 0; }"},
    {ST_PARSE, "enum_nonfold_value",
     "package main; int k; enum { A = *k }; int main(){return 0;}"},
    {ST_PARSE, "chained_desig_field",
     "package main; struct S { struct { int x; } inner; };"
     " int main() { struct S s = { .inner. = 1 }; return 0; }"},
    {ST_PARSE, "chained_desig_index",
     "package main; int main() { int a[2][2] = { [0][x] = 1 }; return 0; }"},
    {ST_PARSE, "chained_desig_float",
     "package main; int main() { int a[2][2] = { [0][1.5] = 1 }; return 0; }"},
    {ST_PARSE, "desig_float_index",
     "package main; int main() { int a[2] = { [1.5] = 1 }; return 0; }"},
    {ST_PARSE, "desig_range_float",
     "package main; int main() { int a[4] = { [0 ... 1.5] = 1 }; return 0; }"},
    {ST_PARSE, "init_eof", "package main; int main() { int a[2] = {"},
    {ST_PARSE, "typedef_ident", "package main; typedef int 1; int main(){return 0;}"},
    {ST_PARSE, "ptr_fn_name", "package main; int *1(void) { return 0; }"},
    {ST_PARSE, "param_after_type",
     "package main; int f(int a, 1) { return 0; } int main(){return 0;}"},
    {ST_PARSE, "grouped_not_fn", "package main; int (*x) { return 0; }"},
    {ST_PARSE, "grouped_unnamed_fn", "package main; int (*()) { return 0; }"},
    {ST_PARSE, "ptr_grouped_unnamed_fn", "package main; int *(*()) { return 0; }"},
    {ST_PARSE, "implicit_int_grouped_fn", "package main; (*foo()) { return 0; }"},

    /* ---- sema ---- */
    {ST_SEMA, "no_main", "package other; int f(void) { return 0; }"},
    {ST_SEMA, "fn_redef", "package main; int main() { return 0; } int main() { return 1; }"},
    {ST_SEMA, "global_redef", "package main; int x = 1; int x = 2; int main(){return 0;}"},
    {ST_SEMA, "static_after_extern", "package main; int x; static int x; int main(){return 0;}"},
    {ST_SEMA, "nonstatic_after_static", "package main; static int x; int x; int main(){return 0;}"},
    {ST_SEMA, "global_vs_fn", "package main; int f(void); int f; int main(){return 0;}"},
    {ST_SEMA, "dup_param", "package main; int f(int a, int a) { return a; } int main(){return 0;}"},
    {ST_SEMA, "variadic_arity",
     "package main; int f(int a, ...) { return a; } int main() { return f(); }"},
    {ST_SEMA, "fnptr_variadic_arity",
     "package main; int (*fp)(int a, ...); int main() { return fp(); }"},
    {ST_SEMA, "fnptr_arity",
     "package main; int (*fp)(int a, int b); int main() { return fp(1); }"},
    {ST_SEMA, "ternary_cond_struct",
     "package main; struct S { int x; }; int main() { struct S s; return s ? 1 : 0; }"},
    {ST_SEMA, "ternary_branch_mismatch",
     "package main; struct S { int x; }; struct T { int y; };"
     " int main() { struct S s; struct T t; return 1 ? s : t; }"},
    {ST_SEMA, "and_left_struct",
     "package main; struct S { int x; }; int main() { struct S s; return s && 1; }"},
    {ST_SEMA, "and_right_struct",
     "package main; struct S { int x; }; int main() { struct S s; return 1 && s; }"},
    {ST_SEMA, "or_left_struct",
     "package main; struct S { int x; }; int main() { struct S s; return s || 1; }"},
    {ST_SEMA, "or_right_struct",
     "package main; struct S { int x; }; int main() { struct S s; return 1 || s; }"},
    {ST_SEMA, "decimal_mix",
     "package main; int main() { _Decimal64 d = 1.DD; return (int)(d + 1.0); }"},
    {ST_SEMA, "decimal_cmp_mix",
     "package main; int main() { _Decimal64 d = 1.DD; return d < 1.0; }"},
    {ST_SEMA, "bitand_left_float", "package main; int main() { return 1.0 & 2; }"},
    {ST_SEMA, "bitand_right_float", "package main; int main() { return 1 & 2.0; }"},
    {ST_SEMA, "bitor_left_float", "package main; int main() { return 1.0 | 2; }"},
    {ST_SEMA, "bitxor_right_float", "package main; int main() { return 1 ^ 2.0; }"},
    {ST_SEMA, "shl_right_float", "package main; int main() { return 1 << 2.0; }"},
    {ST_SEMA, "shr_left_float", "package main; int main() { return 1.0 >> 2; }"},
    {ST_SEMA, "assign_eq_rvalue", "package main; int main() { 1 = 2; return 0; }"},
    {ST_SEMA, "mul_struct",
     "package main; struct S { int x; }; int main() { struct S s; return s * 2; }"},
    {ST_SEMA, "fn_arity",
     "package main; int f(int a, int b) { return a; } int main() { return f(1); }"},
    {ST_SEMA, "fnptr_call_arity",
     "package main; int main() { int (*fp)(int a, int b); return (*fp)(1); }"},
    {ST_SEMA, "fnptr_call_variadic",
     "package main; int main() { int (*fp)(int a, ...); return (*fp)(); }"},
    {ST_SEMA, "float_bit_assign", "package main; int main() { float x; x &= 1; return 0; }"},
    {ST_SEMA, "float_vector_bit_assign",
     "package main; typedef float V __attribute__((vector_size(16)));"
     " int main() { V x; x &= 1; return 0; }"},
    {ST_SEMA, "incomplete_struct_member",
     "package main; int main() { struct NeverDefined x; return x.foo; }"},
    {ST_SEMA, "incomplete_struct_init",
     "package main; int main() { struct NeverDefined x = {1}; return 0; }"},
    {ST_SEMA, "sync_not_ptr", "package main; int main() { return __sync_fetch_and_add(1); }"},
    {ST_SEMA, "tentative_then_nonconst",
     "package main; int f(void); int x; int x = f(); int main(){return 0;}"},
    {ST_SEMA, "mod_float", "package main; int main() { return 1.0 % 2; }"},
    {ST_SEMA, "bitnot_float", "package main; int main() { return ~1.0; }"},
    {ST_SEMA, "not_struct",
     "package main; struct S { int x; }; int main() { struct S s; return !s; }"},
    {ST_SEMA, "assign_rvalue", "package main; int main() { 1 += 2; return 0; }"},
    {ST_SEMA, "assign_const", "package main; int main() { const int x = 1; x += 1; return x; }"},
    {ST_SEMA, "ptr_add_float", "package main; int main() { int *p; p += 1.5; return 0; }"},
    {ST_SEMA, "and_assign_float", "package main; int main() { int x; x &= 1.0; return x; }"},
    {ST_SEMA, "syscall_arity", "package main; int main() { return __syscall(); }"},
    {ST_SEMA, "clone_arity", "package main; int main() { return __clone(); }"},
    {ST_SEMA, "conj_arity", "package main; int main() { return __builtin_conj(); }"},
    {ST_SEMA, "shuffle_arity", "package main; int main() { return __builtin_shuffle(); }"},
    {ST_SEMA, "ctzll_arity", "package main; int main() { return __builtin_ctzll(); }"},
    {ST_SEMA, "atomic_arity", "package main; int main() { return __atomic_load(); }"},
    {ST_SEMA, "atomic_not_ptr", "package main; int main() { return __atomic_load(1); }"},
    {ST_SEMA, "sync_arity", "package main; int main() { return __sync_fetch_and_add(); }"},
    {ST_SEMA, "apply_args_arity", "package main; int main() { return __builtin_apply_args(1); }"},
    {ST_SEMA, "apply_arity", "package main; int main() { return __builtin_apply(); }"},
    {ST_SEMA, "builtin_return_arity", "package main; int main() { __builtin_return(); return 0; }"},
    {ST_SEMA, "va_start_arity", "package main; int main() { va_start(); return 0; }"},
    {ST_SEMA, "va_start_not_list", "package main; int main() { int x; va_start(x); return 0; }"},
    {ST_SEMA, "va_arg_arity", "package main; int main() { va_arg(); return 0; }"},
    {ST_SEMA, "va_arg_not_list", "package main; int main() { int x; return va_arg(x, int); }"},
    {ST_SEMA, "va_end_arity", "package main; int main() { va_end(); return 0; }"},
    {ST_SEMA, "va_end_not_list", "package main; int main() { int x; va_end(x); return 0; }"},
    {ST_SEMA, "va_copy_arity", "package main; int main() { va_copy(); return 0; }"},
    {ST_SEMA, "va_copy_not_list", "package main; int main() { int x, y; va_copy(x, y); return 0; }"},
    {ST_SEMA, "subscript_int", "package main; int main() { int x; return x[0]; }"},
    {ST_SEMA, "member_on_int", "package main; int main() { int x; return x.a; }"},
    {ST_SEMA, "missing_member",
     "package main; struct S { int a; }; int main() { struct S s; return s.no; }"},
    {ST_SEMA, "inc_rvalue", "package main; int main() { ++1; return 0; }"},
    {ST_SEMA, "inc_const", "package main; int main() { const int x = 1; ++x; return x; }"},
    {ST_SEMA, "inc_struct",
     "package main; struct S { int a; }; int main() { struct S s; ++s; return 0; }"},
    {ST_SEMA, "addr_rvalue", "package main; int main() { return (int)&1; }"},
    {ST_SEMA, "dowhile_struct",
     "package main; int main() { do {} while ((struct { int x; }){0}); return 0; }"},
    {ST_SEMA, "switch_float",
     "package main; int main() { switch (1.0) { default: break; } return 0; }"},
    {ST_SEMA, "goto_undeclared", "package main; int main() { goto missing; return 0; }"},
    {ST_SEMA, "void_var", "package main; int main() { void x; return 0; }"},
    {ST_SEMA, "local_redecl", "package main; int main() { int x; int x; return 0; }"},
    {ST_SEMA, "bare_return", "package main; int main() { return; }"},
    {ST_SEMA, "void_return_value", "package main; void f(void) { return 1; } int main(){return 0;}"},
    {ST_SEMA, "deref_nonptr", "package main; int main() { int x; return *x; }"},
    {ST_SEMA, "break_outside", "package main; int main() { break; return 0; }"},
    {ST_SEMA, "continue_outside", "package main; int main() { continue; return 0; }"},
    {ST_SEMA, "add_struct",
     "package main; struct S { int x; }; int main() { struct S s; return s + 1; }"},
    {ST_SEMA, "assign_const_eq",
     "package main; int main() { const int x = 1; x = 2; return x; }"},
    {ST_SEMA, "no_return_main", "package main; int main() { int x; x = 1; }"},
    {ST_SEMA, "index_on_struct",
     "package main; struct S { int a; }; int main() { struct S s = { [0] = 1 }; return 0; }"},
    {ST_SEMA, "designator_oob", "package main; int main() { int a[2] = { [3] = 1 }; return 0; }"},
    {ST_SEMA, "member_on_scalar", "package main; int main() { int x = { .a = 1 }; return 0; }"},
    {ST_SEMA, "union_missing_member",
     "package main; union U { int a; int b; }; int main() { union U u = { .no = 1 }; return 0; }"},
    {ST_SEMA, "struct_designator_missing",
     "package main; struct S { int a; }; int main() { struct S s = { .no = 1 }; return 0; }"},
    {ST_SEMA, "global_nonconst",
     "package main; int f(void); int x = f(); int main(){return 0;}"},
    {ST_SEMA, "call_nonfn", "package main; int (*fp)(void); int main() { return 1(); }"},
    {ST_SEMA, "undeclared", "package main; int main() { return zzz; }"},

    /* ---- new bug coverage ---- */
    {ST_SEMA, "excess_initializer", "package main; int main() { int a[2] = {1, 2, 3}; return 0; }"},
    {ST_SEMA, "struct_return_mismatch", "package main; struct A { int x; }; struct B { int x; }; struct B f(void) { struct A a; a.x = 5; return a; } int main() { return 0; }"},
    {ST_SEMA, "ptr_add_ptr", "package main; int main() { int arr[5]; int *p = arr; int *q = p + p; return 0; }"},
    {ST_SEMA, "ptr_add_float", "package main; int main() { int arr[5]; int *p = arr; float f = 1.0f; int *q = p + f; return 0; }"},
    {ST_SEMA, "int_sub_ptr", "package main; int main() { int arr[5]; int *p = arr; int *r = 5 - p; return 0; }"},
    {ST_SEMA, "struct_assign_mismatch", "package main; struct A { int x; }; struct B { int x; }; int main() { struct A a; struct B b; a = b; return 0; }"},
    {ST_SEMA, "array_assign", "package main; int main() { int a[3] = {1,2,3}; int b[3] = {4,5,6}; a = b; return 0; }"},
    {ST_SEMA, "struct_compare_eq", "package main; struct A { int x; }; int main() { struct A a, b; int r = (a == b); return 0; }"},
    {ST_SEMA, "struct_compare_lt", "package main; struct A { int x; }; int main() { struct A a, b; int r = (a < b); return 0; }"},
    {ST_PARSE, "array_neg_size", "package main; int main() { int arr[-5]; return 0; }"},
    {ST_PARSE, "enum_neg_size", "package main; enum { E = -5 }; int main() { int arr[E]; return 0; }"},
    {ST_PARSE, "bitfield_too_wide", "package main; struct S { int x : 33; }; int main() { return 0; }"},
    {ST_PARSE, "bitfield_float", "package main; struct S { float b : 3; }; int main() { return 0; }"},
    {ST_PARSE, "switch_dup_case", "package main; int main() { int x = 1; switch(x) { case 1: return 10; case 1: return 20; } return 0; }"},
    {ST_PARSE, "switch_dup_default", "package main; int main() { int x = 1; switch(x) { default: return 10; default: return 20; } return 0; }"},
    {ST_PARSE, "case_outside_switch", "package main; int main() { case 1: return 0; }"},
    {ST_PARSE, "default_outside_switch", "package main; int main() { default: return 0; }"},
    {ST_PARSE, "case_float", "package main; int main() { int x = 1; switch(x) { case 1.5: return 0; } return 0; }"},
    {ST_PARSE, "struct_dup_member", "package main; struct S { int x; int x; }; int main() { return 0; }"},
    {ST_PARSE, "fam_not_end", "package main; struct S { int arr[]; int x; }; int main() { return 0; }"},
    {ST_PARSE, "enum_dup_const", "package main; enum color { red, green, blue, red }; int main() { return 0; }"},
    {ST_SEMA, "addr_bitfield", "package main; struct S { int a : 3; }; int main() { struct S s; int *p = &s.a; return 0; }"},
    {ST_SEMA, "array_subscript_float", "package main; int main() { int arr[10]; float f = 3.0f; int x = arr[f]; return 0; }"},
    {ST_SEMA, "sizeof_bitfield", "package main; struct S { int a : 3; }; int main() { struct S s; long sz = sizeof(s.a); return 0; }"},
    {ST_SEMA, "sizeof_incomplete", "package main; struct I; int main() { long sz = sizeof(struct I); return 0; }"},
    {ST_SEMA, "alignof_incomplete", "package main; struct S; int main() { long sz = _Alignof(struct S); return 0; }"},
    {ST_SEMA, "if_struct_cond", "package main; struct S { int x; }; int main() { struct S s; if (s) return 1; return 0; }"},
    {ST_SEMA, "while_struct_cond", "package main; struct S { int x; }; int main() { struct S s; while (s) return 1; return 0; }"},
    {ST_SEMA, "for_struct_cond", "package main; struct S { int x; }; int main() { struct S s; for (; s ;) return 1; return 0; }"},
    {ST_SEMA, "dup_label", "package main; int main() { L: ; L: ; return 0; }"},
    {ST_SEMA, "global_conflicting_type", "package main; int i; double i; int main() { return 0; }"},
    {ST_SEMA, "ternary_void_mismatch", "package main; void f(void) {} int g(void) { return 1; } int main() { int x = (1 ? f() : g()); return 0; }"},
    {ST_SEMA, "addr_register_var", "package main; int main() { register int x = 5; int *p = &x; return 0; }"},
    {ST_SEMA, "var_incomplete_type", "package main; struct S; int main() { struct S s; return 0; }"},
    {ST_SEMA, "array_incomplete_type", "package main; struct I; struct I arr[5]; int main() { return 0; }"},
    {ST_SEMA, "array_void_type", "package main; void v[10]; int main() { return 0; }"},
    {ST_SEMA, "array_of_functions", "package main; int farr[5](void); int main() { return 0; }"},
    {ST_SEMA, "param_incomplete_type", "package main; struct I; int f(struct I s) { return 0; } int main() { return 0; }"},
    {ST_SEMA, "static_vla", "package main; int f(int n) { static int a[n]; return 0; } int main() { return 0; }"},
    {ST_PARSE, "align_non_power_two", "package main; int x __attribute__((aligned(3))); int main() { return 0; }"},
    {ST_SEMA, "void_value_assigned", "package main; void f(void) {} int main() { int x = f(); return 0; }"},
    {ST_SEMA, "char_array_str_overflow", "package main; int main() { char s[2] = \"abc\"; return 0; }"},

    /* ---- ir ---- */
    {ST_IR, "va_arg_pack_invalid",
     "package main; int id(int x) { return x; }"
     "int main() { return id(__builtin_va_arg_pack()); }"},
    {ST_IR, "global_var_init_forward",
     "package main; int x = n; int n = 1; int main(){return 0;}"},
    {ST_IR, "global_extern_init",
     "package main; extern int n; int x = n; int main(){return 0;}"},
};

static void test_table(void) {
    size_t i, n = sizeof(k_cases) / sizeof(k_cases[0]);
    for (i = 0; i < n; i++) {
        if (!dies(k_cases[i].stage, k_cases[i].src)) {
            fprintf(stderr, "test_die_at: expected die: %s\n  %s\n",
                    k_cases[i].name, k_cases[i].src);
            T_ASSERT(0);
        } else {
            T_ASSERT(1);
        }
    }
}

static const char *too_many_kr_params(void) {
    static char *buf;
    if (buf)
        return buf;
    size_t cap = 16u * 1026u + 80u;
    buf = malloc(cap);
    if (!buf)
        exit(1);
    char *p = buf;
    p += sprintf(p, "package main; int f(");
    int i;
    for (i = 0; i < 1025; i++) {
        if (i)
            p += sprintf(p, ", ");
        p += sprintf(p, "a%d", i);
    }
    sprintf(p, ") { return 0; } int main() { return 0; }");
    return buf;
}

static const char *nested_va_arg_pack(void) {
    static char *buf;
    if (buf)
        return buf;
    buf = malloc(4096);
    if (!buf)
        exit(1);
    char *p = buf;
    int i;
    p += sprintf(p, "package main;\n");
    p += sprintf(p,
                 "extern inline __attribute__((always_inline, gnu_inline)) "
                 "int w0(int x, ...) {\n"
                 "  (void)__builtin_va_arg_pack_len();\n"
                 "  return x;\n"
                 "}\n");
    for (i = 1; i <= 8; i++) {
        p += sprintf(p,
                     "extern inline __attribute__((always_inline, gnu_inline)) "
                     "int w%d(int x, ...) {\n"
                     "  (void)__builtin_va_arg_pack_len();\n"
                     "  return w%d(x, 1);\n"
                     "}\n",
                     i, i - 1);
    }
    sprintf(p, "int main() { return w8(1); }\n");
    return buf;
}

static const char *too_many_fnptr_params(void) {
    static char *buf;
    if (buf)
        return buf;
    size_t cap = 32u * 1026u + 80u;
    buf = malloc(cap);
    if (!buf)
        exit(1);
    char *p = buf;
    p += sprintf(p, "package main; int (*fp)(");
    int i;
    for (i = 0; i < 1025; i++) {
        if (i)
            p += sprintf(p, ", ");
        p += sprintf(p, "int a%d", i);
    }
    sprintf(p, "); int main() { return 0; }");
    return buf;
}

static void test_generated(void) {
    static const struct {
        const char *name;
        const char *(*mk)(void);
    } extra[] = {
        {"too_many_params", too_many_params},
        {"too_many_kr_params", too_many_kr_params},
        {"too_many_fnptr_params", too_many_fnptr_params},
        {"nested_va_arg_pack", nested_va_arg_pack},
    };
    static const Stage extra_st[] = {ST_PARSE, ST_PARSE, ST_PARSE, ST_IR};
    size_t i;
    for (i = 0; i < sizeof(extra) / sizeof(extra[0]); i++) {
        if (!dies(extra_st[i], extra[i].mk())) {
            fprintf(stderr, "test_die_at: expected die: %s\n", extra[i].name);
            T_ASSERT(0);
        } else {
            T_ASSERT(1);
        }
    }
}

int main(void) {
    test_table();
    test_generated();
    return t_finalize();
}
