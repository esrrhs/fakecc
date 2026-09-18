#include "fakecc/ast.h"
#include "fakecc/common.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

/* ---- helper: lex + parse + sema + ir_generate ---- */
static IRModule compile_to_ir(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 1);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 0);

    token_array_free(&arr);
    tu_free(&tu);

    return ir;
}

/* ---- tests ---- */

static void test_return_zero(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 0; }");
    T_ASSERT_EQ_INT((int)ir.functions.len, 1);
    T_ASSERT_STR_EQ(ir.functions.data[0].name, "main");
    /* Now: IR_CONST v0=0; IR_RETURN v0 */
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 2);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 0);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_RETURN);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[1].a, 0);
    ir_module_free(&ir);
}

static void test_return_42(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 42; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 2);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 42);
    ir_module_free(&ir);
}

static void test_return_255(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 255; }");
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 255);
    ir_module_free(&ir);
}

static void test_function_name_propagated(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 1; }");
    T_ASSERT_STR_EQ(ir.functions.data[0].name, "main");
    ir_module_free(&ir);
}

static void test_source_loc_propagated(void) {
    IRModule ir = compile_to_ir("package main; int main() { return 42; }");
    T_ASSERT(ir.functions.data[0].insts.data[0].loc.line > 0);
    ir_module_free(&ir);
}

/* ---- Slice 2: expression IR tests ---- */

static void test_add_ir(void) {
    /* return 1+2; → CONST v0=1; CONST v1=2; ADD v2=v0+v1; RETURN v2 */
    IRModule ir = compile_to_ir("package main; int main() { return 1 + 2; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 4);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 1);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[1].imm, 2);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_ADD);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[3].op, (int)IR_RETURN);
    ir_module_free(&ir);
}

static void test_neg_ir(void) {
    /* return -5; → CONST v0=5; NEG v1=v0; RETURN v1 */
    IRModule ir = compile_to_ir("package main; int main() { return -5; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 3);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[0].imm, 5);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_NEG);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_RETURN);
    ir_module_free(&ir);
}

static void test_mul_ir(void) {
    /* return 2*3; → CONST v0=2; CONST v1=3; MUL v2=v0*v1; RETURN v2 */
    IRModule ir = compile_to_ir("package main; int main() { return 2 * 3; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_MUL);
    ir_module_free(&ir);
}

static void test_div_mod_ir(void) {
    /* return 17%5; → ... MOD ...; RETURN */
    IRModule ir = compile_to_ir("package main; int main() { return 17 % 5; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_MOD);
    ir_module_free(&ir);

    IRModule ir2 = compile_to_ir("package main; int main() { return 20 / 4; }");
    T_ASSERT_EQ_INT((int)ir2.functions.data[0].insts.data[2].op, (int)IR_DIV);
    ir_module_free(&ir2);
}

/* ---- Slice 3: variable IR tests ---- */

static void test_var_ir_sequence(void) {
    /* int x; x = 42; return x;
     * → ALLOCA v0; CONST v1=42; STORE a=v0,b=v1; LOAD v2=v0; RETURN v2 */
    IRModule ir = compile_to_ir(
        "package main; int main() { int x; x = 42; return x; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 5);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_ALLOCA);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[1].imm, 42);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_STORE);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[3].op, (int)IR_LOAD);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[4].op, (int)IR_RETURN);
    ir_module_free(&ir);
}

static void test_decl_init_ir(void) {
    /* int x = 5; return x;
     * → ALLOCA v0; CONST v1=5; STORE a=v0,b=v1; LOAD v2=v0; RETURN v2 */
    IRModule ir = compile_to_ir(
        "package main; int main() { int x = 5; return x; }");
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.len, 5);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[0].op, (int)IR_ALLOCA);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[1].op, (int)IR_CONST);
    T_ASSERT_EQ_INT(ir.functions.data[0].insts.data[1].imm, 5);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[2].op, (int)IR_STORE);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[3].op, (int)IR_LOAD);
    T_ASSERT_EQ_INT((int)ir.functions.data[0].insts.data[4].op, (int)IR_RETURN);
    ir_module_free(&ir);
}

/* 16-byte vector_size is one SSE PARAM (width 16), not two eightbytes and
 * not x87 long double. */
static void test_vector16_param_width(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "typedef double V __attribute__((vector_size(16)));"
        "V id(V v) { return v; }"
        "int main(void) { return 0; }");
    T_ASSERT(ir.functions.len >= 1);
    const IRFunction *fn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "id") == 0) {
            fn = &ir.functions.data[i];
            break;
        }
    }
    T_ASSERT(fn != NULL);
    T_ASSERT(fn->insts.len > 0);
    T_ASSERT_EQ_INT((int)fn->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(fn->insts.data[0].width, 16);
    int saw_vec_mem = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IROpcode op = fn->insts.data[i].op;
        if ((op == IR_LOAD_PTR || op == IR_STORE_PTR)
            && fn->insts.data[i].width == 16)
            saw_vec_mem = 1;
    }
    T_ASSERT(saw_vec_mem);
    ir_module_free(&ir);
}

/* 32-byte vector_size is one SSE PARAM (width 32 / YMM), not MEMORY. */
static void test_vector32_param_width(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "typedef int V __attribute__((vector_size(32)));"
        "V id(V v) { return v; }"
        "int main(void) { return 0; }");
    T_ASSERT(ir.functions.len >= 1);
    const IRFunction *fn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "id") == 0) {
            fn = &ir.functions.data[i];
            break;
        }
    }
    T_ASSERT(fn != NULL);
    T_ASSERT(fn->insts.len > 0);
    T_ASSERT_EQ_INT((int)fn->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(fn->insts.data[0].width, 32);
    int saw_vec_mem = 0;
    for (size_t i = 0; i < fn->insts.len; i++) {
        IROpcode op = fn->insts.data[i].op;
        if ((op == IR_LOAD_PTR || op == IR_STORE_PTR)
            && fn->insts.data[i].width == 32)
            saw_vec_mem = 1;
    }
    T_ASSERT(saw_vec_mem);
    ir_module_free(&ir);
}

/* -mno-avx: 32-byte vector_size is MEMORY (no YMM), not one SSE PARAM. */
static void test_vector32_mno_avx_is_memory(void) {
    int saved = g_no_avx;
    g_no_avx = 1;
    IRModule ir = compile_to_ir(
        "package main;"
        "typedef int V __attribute__((vector_size(32)));"
        "int take(V v) { return v[0] + v[7]; }"
        "int main(void) { V v = {1,0,0,0,0,0,0,6}; return take(v); }");
    const IRFunction *take = NULL;
    const IRFunction *mainfn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "main") == 0)
            mainfn = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    T_ASSERT(mainfn != NULL);
    T_ASSERT(take->insts.len > 0);
    T_ASSERT_EQ_INT((int)take->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 1);
    T_ASSERT_EQ_INT(take->insts.data[0].alloca_bytes, 32);
    int saw_blob = 0;
    for (size_t i = 0; i < mainfn->insts.len; i++) {
        const IRInst *inst = &mainfn->insts.data[i];
        if (inst->op != IR_CALL || !inst->call_name) continue;
        if (strcmp(inst->call_name, "take") != 0) continue;
        T_ASSERT(inst->call_arg_on_stack != NULL);
        T_ASSERT(inst->call_nargs >= 1);
        T_ASSERT((inst->call_arg_on_stack[0] & CALL_ARG_BLOB) != 0);
        T_ASSERT(inst->call_arg_nbytes != NULL);
        T_ASSERT_EQ_INT(inst->call_arg_nbytes[0], 32);
        saw_blob = 1;
    }
    T_ASSERT(saw_blob);
    ir_module_free(&ir);
    g_no_avx = saved;
}

/* aligned(32) { vector_size(16) } is MEMORY (padding is NO_CLASS), not YMM. */
static void test_overaligned_sse_is_memory_blob(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "typedef double V __attribute__((vector_size(16)));"
        "struct __attribute__((aligned(32))) A { V x; };"
        "int take(struct A a) { return (int)a.x[0]; }"
        "int main(void) { struct A a; a.x[0] = 1.0; a.x[1] = 2.0; return take(a); }");
    T_ASSERT(ir.functions.len >= 2);
    const IRFunction *take = NULL;
    const IRFunction *mainfn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "main") == 0)
            mainfn = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    T_ASSERT(mainfn != NULL);
    T_ASSERT(take->insts.len > 0);
    T_ASSERT_EQ_INT((int)take->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 1);
    T_ASSERT_EQ_INT(take->insts.data[0].alloca_bytes, 32);

    int saw_blob = 0;
    for (size_t i = 0; i < mainfn->insts.len; i++) {
        const IRInst *inst = &mainfn->insts.data[i];
        if (inst->op != IR_CALL || !inst->call_name) continue;
        if (strcmp(inst->call_name, "take") != 0) continue;
        T_ASSERT(inst->call_arg_on_stack != NULL);
        T_ASSERT(inst->call_nargs >= 1);
        T_ASSERT((inst->call_arg_on_stack[0] & CALL_ARG_BLOB) != 0);
        T_ASSERT(inst->call_arg_nbytes != NULL);
        T_ASSERT_EQ_INT(inst->call_arg_nbytes[0], 32);
        saw_blob = 1;
    }
    T_ASSERT(saw_blob);
    ir_module_free(&ir);
}

/* FAM `{ int n; char d[]; }` is INTEGER (fixed prefix), not a MEMORY blob. */
static void test_fam_prefix_is_integer(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "struct F { int n; char d[]; };"
        "int take(struct F f, int k) { return f.n + k; }"
        "int main(void) { struct F f; f.n = 1; return take(f, 7); }");
    const IRFunction *take = NULL;
    const IRFunction *mainfn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "main") == 0)
            mainfn = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    T_ASSERT(mainfn != NULL);
    T_ASSERT(take->insts.len > 0);
    T_ASSERT_EQ_INT((int)take->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 0);

    int saw_call = 0;
    for (size_t i = 0; i < mainfn->insts.len; i++) {
        const IRInst *inst = &mainfn->insts.data[i];
        if (inst->op != IR_CALL || !inst->call_name) continue;
        if (strcmp(inst->call_name, "take") != 0) continue;
        T_ASSERT(inst->call_nargs >= 2);
        if (inst->call_arg_on_stack)
            T_ASSERT((inst->call_arg_on_stack[0] & CALL_ARG_BLOB) == 0);
        saw_call = 1;
    }
    T_ASSERT(saw_call);
    ir_module_free(&ir);
}

/* `{ char x[0]; }` occupies no slots: 6 GP ints + Z + h → 7 PARAMs, h on stack. */
static void test_zero_length_struct_no_slot(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "struct Z { char x[0]; };"
        "int take6z(int a, int b, int c, int d, int e, int f, struct Z z, int h) {"
        "  return h; }"
        "int main(void) { struct Z z; return take6z(1,2,3,4,5,6,z,99); }");
    const IRFunction *take = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take6z") == 0)
            take = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    int nparam = 0;
    for (size_t i = 0; i < take->insts.len; i++) {
        if (take->insts.data[i].op != IR_PARAM) break;
        nparam++;
    }
    T_ASSERT_EQ_INT(nparam, 7);
    ir_module_free(&ir);
}

/* 16-byte `{ long double }` returns in st0: no hidden sret PARAM. */
static void test_x87_agg_ret_no_sret(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "struct Ld { long double x; };"
        "struct Ld make(long double v) { struct Ld s; s.x = v; return s; }"
        "int main(void) { struct Ld s = make(1.0L); return (int)s.x; }");
    const IRFunction *make = NULL;
    const IRFunction *mainfn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "make") == 0)
            make = &ir.functions.data[i];
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "main") == 0)
            mainfn = &ir.functions.data[i];
    }
    T_ASSERT(make != NULL);
    T_ASSERT(mainfn != NULL);
    T_ASSERT_EQ_INT(make->ret_x87_bytes, 16);
    T_ASSERT_EQ_INT(make->sret_value, -1);
    int saw_x87_ret = 0;
    for (size_t i = 0; i < make->insts.len; i++) {
        const IRInst *inst = &make->insts.data[i];
        if (inst->op != IR_RETURN) continue;
        T_ASSERT_EQ_INT(inst->x87_pair, 1);
        T_ASSERT_EQ_INT((int)inst->imm, 16);
        saw_x87_ret = 1;
    }
    T_ASSERT(saw_x87_ret);

    int saw_x87_call = 0;
    for (size_t i = 0; i < mainfn->insts.len; i++) {
        const IRInst *inst = &mainfn->insts.data[i];
        if (inst->op != IR_CALL || !inst->call_name) continue;
        if (strcmp(inst->call_name, "make") != 0) continue;
        T_ASSERT_EQ_INT(inst->x87_pair, 1);
        T_ASSERT_EQ_INT((int)inst->imm, 16);
        T_ASSERT_EQ_INT(inst->dst, -1);
        saw_x87_call = 1;
    }
    T_ASSERT(saw_x87_call);
    ir_module_free(&ir);
}

/* `union { int n; char d[]; }` is MEMORY; `[0]` stays INTEGER. */
static void test_union_fam_is_memory(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "union U { int n; char d[]; };"
        "int take(union U u, int k) { return u.n + k; }"
        "int main(void) { union U u; u.n = 1; return take(u, 7); }");
    const IRFunction *take = NULL;
    const IRFunction *mainfn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "main") == 0)
            mainfn = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    T_ASSERT(mainfn != NULL);
    T_ASSERT(take->insts.len > 0);
    T_ASSERT_EQ_INT((int)take->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 1);
    T_ASSERT_EQ_INT(take->insts.data[0].alloca_bytes, 4);

    int saw_blob = 0;
    for (size_t i = 0; i < mainfn->insts.len; i++) {
        const IRInst *inst = &mainfn->insts.data[i];
        if (inst->op != IR_CALL || !inst->call_name) continue;
        if (strcmp(inst->call_name, "take") != 0) continue;
        T_ASSERT(inst->call_arg_on_stack != NULL);
        T_ASSERT(inst->call_nargs >= 1);
        T_ASSERT((inst->call_arg_on_stack[0] & CALL_ARG_BLOB) != 0);
        saw_blob = 1;
    }
    T_ASSERT(saw_blob);
    ir_module_free(&ir);
}

static void test_union_zero_array_is_integer(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "union U { int n; char d[0]; };"
        "int take(union U u, int k) { return u.n + k; }"
        "int main(void) { union U u; u.n = 1; return take(u, 7); }");
    const IRFunction *take = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    T_ASSERT(take->insts.len > 0);
    T_ASSERT_EQ_INT((int)take->insts.data[0].op, (int)IR_PARAM);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 0);
    ir_module_free(&ir);
}

/* Trailing NO_CLASS padding is not INTEGER: aligned(16) { long } is one GP. */
static void test_aligned16_pad_one_gp(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "struct __attribute__((aligned(16))) S { long x; };"
        "int take(struct S s, int k) { return (int)s.x + k; }"
        "int main(void) { struct S s; s.x = 3; return take(s, 4); }");
    const IRFunction *take = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    int nparam = 0;
    for (size_t i = 0; i < take->insts.len; i++) {
        if (take->insts.data[i].op != IR_PARAM) break;
        nparam++;
    }
    T_ASSERT_EQ_INT(nparam, 2);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 0);
    ir_module_free(&ir);
}

/* Mixed union { long double; long long } returns via sret, not st0. */
static void test_mixed_ld_union_uses_sret(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "union U { long double d; long long l; };"
        "union U make(long double v) { union U u; u.d = v; return u; }"
        "int main(void) { union U u = make(1.0L); return (int)u.d; }");
    const IRFunction *make = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "make") == 0)
            make = &ir.functions.data[i];
    }
    T_ASSERT(make != NULL);
    T_ASSERT_EQ_INT(make->ret_x87_bytes, 0);
    T_ASSERT(make->sret_value >= 0);
    T_ASSERT(make->insts.len > 0);
    T_ASSERT_EQ_INT((int)make->insts.data[0].op, (int)IR_PARAM);
    ir_module_free(&ir);
}

/* Packed `:60` + `:8` spills into the next eightbyte: two GP params + k. */
static void test_packed_bitfield_spill_two_gp(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "struct __attribute__((packed)) P { unsigned long a:60; unsigned b:8; };"
        "int take(struct P p, int k) { return (int)p.b + k; }"
        "int main(void) { struct P p; p.a = 1; p.b = 2; return take(p, 5); }");
    const IRFunction *take = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "take") == 0)
            take = &ir.functions.data[i];
    }
    T_ASSERT(take != NULL);
    int nparam = 0;
    for (size_t i = 0; i < take->insts.len; i++) {
        if (take->insts.data[i].op != IR_PARAM) break;
        nparam++;
    }
    T_ASSERT_EQ_INT(nparam, 3);
    T_ASSERT_EQ_INT(take->insts.data[0].force_stack, 0);
    T_ASSERT_EQ_INT(take->insts.data[1].force_stack, 0);
    ir_module_free(&ir);
}

/* File-scope `aligned(16)` is recorded on the IR global; a matching local
 * pinned alloca carries the alignment in IR_ALLOCA.imm. */
static void test_aligned16_storage_ir(void) {
    IRModule ir = compile_to_ir(
        "package main;"
        "char pad;"
        "int g __attribute__((aligned(16)));"
        "int main(void) {"
        "  char c; int x __attribute__((aligned(16)));"
        "  return (int)((unsigned long)&x & 15ul)"
        "       + (int)((unsigned long)&g & 15ul);"
        "}");
    const IRGlobal *g = NULL;
    for (size_t i = 0; i < ir.globals.len; i++) {
        if (ir.globals.data[i].name &&
            strcmp(ir.globals.data[i].name, "g") == 0)
            g = &ir.globals.data[i];
    }
    T_ASSERT(g != NULL);
    T_ASSERT_EQ_INT(g->align, 16);
    const IRFunction *main_fn = NULL;
    for (size_t i = 0; i < ir.functions.len; i++) {
        if (ir.functions.data[i].name &&
            strcmp(ir.functions.data[i].name, "main") == 0)
            main_fn = &ir.functions.data[i];
    }
    T_ASSERT(main_fn != NULL);
    int saw_aln = 0;
    for (size_t i = 0; i < main_fn->insts.len; i++) {
        if (main_fn->insts.data[i].op == IR_ALLOCA
            && main_fn->insts.data[i].imm >= 16)
            saw_aln = 1;
    }
    T_ASSERT_EQ_INT(saw_aln, 1);
    ir_module_free(&ir);
}

/* ---- main ---- */

int main(void) {
    test_return_zero();
    test_return_42();
    test_return_255();
    test_function_name_propagated();
    test_source_loc_propagated();
    test_add_ir();
    test_neg_ir();
    test_mul_ir();
    test_div_mod_ir();
    test_var_ir_sequence();
    test_decl_init_ir();
    test_vector16_param_width();
    test_vector32_param_width();
    test_vector32_mno_avx_is_memory();
    test_overaligned_sse_is_memory_blob();
    test_fam_prefix_is_integer();
    test_zero_length_struct_no_slot();
    test_x87_agg_ret_no_sret();
    test_union_fam_is_memory();
    test_union_zero_array_is_integer();
    test_aligned16_pad_one_gp();
    test_mixed_ld_union_uses_sret();
    test_packed_bitfield_spill_two_gp();
    test_aligned16_storage_ir();
    return t_finalize();
}
