#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/target.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/* ---- helper: find a defined symbol by name ---- */
static const EmitSymbol *find_sym(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_syms; i++) {
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0)
            return &m->syms[i];
    }
    return NULL;
}

/* ---- helper: compile source to EmitModule ---- */
static EmitModule compile_to_code(const char *src) {
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

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);

    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);

    return em;
}

/* Full -O0 pipeline: pin locals, RA, then isel. */
static EmitModule compile_o0(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 1);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 1);
    opt(&ir, 0, 0);

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);

    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);

    return em;
}

/* ---- tests ---- */

static void test_return_zero(void) {
    EmitModule em = compile_to_code("package main; int main() { return 0; }");
    const EmitSymbol *main_sym = find_sym(&em, "main");
    T_ASSERT(main_sym != NULL);
    /* With stack evaluation, code is longer than Slice 1's 11 bytes.
     * Just verify non-empty and a minimum size. */
    T_ASSERT(main_sym->size > 0);
    T_ASSERT(em.text.len > 0);
    emit_module_free(&em);
}

static void test_return_42(void) {
    EmitModule em = compile_to_code("package main; int main() { return 42; }");
    T_ASSERT(find_sym(&em, "main")->size > 0);
    T_ASSERT(em.text.len > 0);
    emit_module_free(&em);
}

static void test_return_255(void) {
    EmitModule em = compile_to_code("package main; int main() { return 255; }");
    T_ASSERT(find_sym(&em, "main")->size > 0);
    emit_module_free(&em);
}

static void test_prologue_present(void) {
    EmitModule em = compile_to_code("package main; int main() { return 1; }");
    /* pushq %rbp = 55 */
    T_ASSERT((unsigned char)em.text.data[0] == 0x55);
    /* movq %rsp, %rbp = 48 89 e5 */
    T_ASSERT((unsigned char)em.text.data[1] == 0x48);
    T_ASSERT((unsigned char)em.text.data[2] == 0x89);
    T_ASSERT((unsigned char)em.text.data[3] == 0xe5);
    /* sub $N, %rsp = 48 83 EC ib (imm8) or 48 81 EC id (imm32) */
    T_ASSERT((unsigned char)em.text.data[4] == 0x48);
    T_ASSERT((unsigned char)em.text.data[5] == 0x83
             || (unsigned char)em.text.data[5] == 0x81);
    T_ASSERT((unsigned char)em.text.data[6] == 0xEC);
    emit_module_free(&em);
}

/* ---- Slice 3: variable codegen tests ---- */

static void test_var_codegen(void) {
    /* int x; x = 42; return x; — non-empty code + correct prologue */
    EmitModule em = compile_to_code(
        "package main; int main() { int x; x = 42; return x; }");
    const EmitSymbol *main_sym = find_sym(&em, "main");
    T_ASSERT(main_sym != NULL);
    T_ASSERT(main_sym->size > 0);
    T_ASSERT(em.text.len > 0);
    /* prologue still intact */
    T_ASSERT((unsigned char)em.text.data[0] == 0x55);
    T_ASSERT((unsigned char)em.text.data[1] == 0x48);
    T_ASSERT((unsigned char)em.text.data[2] == 0x89);
    T_ASSERT((unsigned char)em.text.data[3] == 0xe5);
    emit_module_free(&em);
}

static void test_arith_codegen_longer(void) {
    EmitModule simple = compile_to_code(
        "package main; int main() { return 1; }");
    EmitModule arith = compile_to_code(
        "package main; int main() { return 1 + 2 * 3 - 4; }");
    T_ASSERT(arith.text.len > simple.text.len);
    T_ASSERT(find_sym(&arith, "main") != NULL);
    emit_module_free(&simple);
    emit_module_free(&arith);
}

static void test_if_codegen_has_main(void) {
    EmitModule em = compile_to_code(
        "package main; int main() { int x = 1; if (x) return 2; return 3; }");
    const EmitSymbol *main_sym = find_sym(&em, "main");
    T_ASSERT(main_sym != NULL);
    T_ASSERT(main_sym->size > 16);
    emit_module_free(&em);
}

static void test_multi_function_symbols(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int add(int a, int b) { return a + b; }"
        "int main() { return add(1, 2); }");
    T_ASSERT(find_sym(&em, "main") != NULL);
    T_ASSERT(find_sym(&em, "add") != NULL);
    emit_module_free(&em);
}

static void test_bitfield_codegen(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "struct F { unsigned a : 3; unsigned b : 5; };"
        "int main() { struct F f; f.a = 1; f.b = 2; return f.a + f.b; }");
    T_ASSERT(find_sym(&em, "main") != NULL);
    T_ASSERT(em.text.len > 0);
    emit_module_free(&em);
}

static int text_has_sib(const EmitModule *em, unsigned char sib) {
    /* ModRM.rm == 4 means a SIB byte follows. */
    for (size_t i = 1; i < em->text.len; i++) {
        unsigned char modrm = (unsigned char)em->text.data[i - 1];
        if ((modrm & 7) == 4 && (unsigned char)em->text.data[i] == sib)
            return 1;
    }
    return 0;
}

/* scale=4, base=none (abs32), any index: (sib & 0xC7) == 0x85. */
static int text_has_sib_abs32_scale4(const EmitModule *em) {
    for (size_t i = 1; i < em->text.len; i++) {
        unsigned char modrm = (unsigned char)em->text.data[i - 1];
        unsigned char sib = (unsigned char)em->text.data[i];
        if ((modrm & 7) == 4 && (sib & 0xC7) == 0x85)
            return 1;
    }
    return 0;
}

/* scale=4, base=rbp/r13, any index: (sib & 0xC7) == 0x85 with mod!=0 —
 * equivalently (sib & 0xC7) == 0x85 is also used for abs32; local form
 * pairs with mod=1/2 and base field 101 already in the SIB base nibble. */
static int text_has_sib_rbp_scale4(const EmitModule *em) {
    for (size_t i = 1; i < em->text.len; i++) {
        unsigned char modrm = (unsigned char)em->text.data[i - 1];
        unsigned char sib = (unsigned char)em->text.data[i];
        if ((modrm & 7) != 4) continue;
        /* scale=4, base=rbp (5): (sib & 0xC7) == 0x85 — same mask as abs32
         * but mod != 0 distinguishes [rbp+idx*4+disp]. */
        if ((sib & 0xC7) == 0x85 && (modrm & 0xC0) != 0)
            return 1;
        /* legacy scale=1 index=* base=rbp → 0x0D family */
        if ((sib & 0xC7) == 0x05 && (modrm & 0xC0) != 0)
            return 1;
    }
    return 0;
}

/* g[i] at -O0 should be a SIB load, not lea+add+indirect.
 * Prefer abs32 g(,%reg,4); older [rax+rcx] forms remain acceptable. */
static void test_sib_global_index(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int g[8];"
        "int idx(int i) { return g[i]; }"
        "int main() { return idx(3); }");
    T_ASSERT(text_has_sib_abs32_scale4(&em)
             || text_has_sib(&em, 0x08) || text_has_sib(&em, 0x01));
    emit_module_free(&em);
}

/* a[i] of a pinned local should be [rbp+reg*4+off] (or scale-1 equivalent). */
static void test_sib_local_index(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int f(int i) { int a[8]; a[0] = 1; a[i] = 2; return a[i]; }"
        "int main() { return f(1); }");
    T_ASSERT(text_has_sib_rbp_scale4(&em) || text_has_sib(&em, 0x0D));
    emit_module_free(&em);
}

/* 0F 10/11 without F2/F3/66 is movups — required to move a 16-byte vector
 * through one XMM.  Scalar float uses F2/F3 prefixes (movsd/movss). */
static int text_has_unprefixed_0f(const EmitModule *em, unsigned char op) {
    for (size_t i = 0; i + 1 < em->text.len; i++) {
        if ((unsigned char)em->text.data[i] != 0x0F) continue;
        if ((unsigned char)em->text.data[i + 1] != op) continue;
        int pref = 0;
        if (i > 0) {
            unsigned char p = (unsigned char)em->text.data[i - 1];
            if (p == 0xF2 || p == 0xF3 || p == 0x66) pref = 1;
            if (!pref && i > 1 && (p & 0xF0) == 0x40) {
                unsigned char p2 = (unsigned char)em->text.data[i - 2];
                if (p2 == 0xF2 || p2 == 0xF3 || p2 == 0x66) pref = 1;
            }
        }
        if (!pref) return 1;
    }
    return 0;
}

static void test_vector16_uses_movups(void) {
    EmitModule em = compile_o0(
        "package main;"
        "typedef double V __attribute__((vector_size(16)));"
        "V id(V v) { return v; }"
        "int main(void) { V a = { 1.0, 2.0 }; V b = id(a); return (int)b[0]; }");
    T_ASSERT(find_sym(&em, "id") != NULL);
    T_ASSERT(text_has_unprefixed_0f(&em, 0x10)
             || text_has_unprefixed_0f(&em, 0x11));
    emit_module_free(&em);
}

static void test_constructor_init_array(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int g;"
        "__attribute__((constructor)) void ctor(void) { g = 7; }"
        "int main(void) { return g; }");
    T_ASSERT(find_sym(&em, "ctor") != NULL);
    T_ASSERT_EQ_INT((int)em.init_array.len, 8);
    T_ASSERT(em.num_data_relocs >= 1);
    int saw = 0;
    for (size_t i = 0; i < em.num_data_relocs; i++) {
        if (em.data_relocs[i].shndx == SECT_INIT_ARRAY
            && em.data_relocs[i].type == R_X86_64_64)
            saw = 1;
    }
    T_ASSERT(saw);
    emit_module_free(&em);
}

static void test_destructor_fini_array(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int g;"
        "__attribute__((destructor)) void dtor(void) { g = 1; }"
        "int main(void) { return g; }");
    T_ASSERT(find_sym(&em, "dtor") != NULL);
    T_ASSERT_EQ_INT((int)em.fini_array.len, 8);
    int saw = 0;
    for (size_t i = 0; i < em.num_data_relocs; i++) {
        if (em.data_relocs[i].shndx == SECT_FINI_ARRAY
            && em.data_relocs[i].type == R_X86_64_64)
            saw = 1;
    }
    T_ASSERT(saw);
    emit_module_free(&em);
}

static void test_constructor_priority_slots(void) {
    EmitModule em = compile_to_code(
        "package main;"
        "int g;"
        "__attribute__((constructor(200))) void ctor_b(void) { g = 2; }"
        "__attribute__((constructor(101))) void ctor_a(void) { g = 1; }"
        "int main(void) { return g; }");
    T_ASSERT_EQ_INT((int)em.init_array.len, 16);
    T_ASSERT(em.init_prio != NULL);
    T_ASSERT_EQ_INT(em.init_prio[0], 200);
    T_ASSERT_EQ_INT(em.init_prio[1], 101);
    emit_module_free(&em);
}

static IRModule compile_src_to_ir(const char *src) {
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

static void test_rol8_encoding(void) {
    IRModule ir = compile_src_to_ir(
        "package main; unsigned rot(unsigned x, unsigned n) {"
        "  return (x << n) | (x >> (32u - n)); }"
        "int main() { return (int)rot(1u, 1u); }");
    int found = 0;
    for (size_t f = 0; f < ir.functions.len; f++) {
        IRFunction *fn = &ir.functions.data[f];
        for (size_t i = 0; i < fn->insts.len; i++) {
            if (fn->insts.data[i].op == IR_ROL) {
                fn->insts.data[i].width = 1;
                found = 1;
            }
        }
    }
    T_ASSERT(found);
    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);
    int saw_d2 = 0;
    for (size_t i = 0; i < em.text.len; i++) {
        if ((unsigned char)em.text.data[i] == 0xD2) saw_d2 = 1;
    }
    T_ASSERT(saw_d2);
    emit_module_free(&em);
    ir_module_free(&ir);
}

static void test_builtin_return_codegen(void) {
    EmitModule em = compile_to_code(
        "package main; int add1(int x) { return x + 1; }"
        "int wrap(int x) {"
        "  void *args = __builtin_apply_args();"
        "  __builtin_return(__builtin_apply((void (*)())add1, args, 16));"
        "} int main() { return wrap(41); }");
    T_ASSERT(find_sym(&em, "wrap") != NULL);
    T_ASSERT(find_sym(&em, "wrap")->size > 0);
    emit_module_free(&em);
}


/* A 6-int O0 leaf that only adds its parameters should not push unused
 * callee-saved regs (those colors may still sit on fused-away SSA). */
static int fn_text_has_push_cs(const EmitModule *em, const char *name) {
    const EmitSymbol *s = find_sym(em, name);
    if (!s || s->value + s->size > em->text.len) return 1;
    const unsigned char *b = (const unsigned char *)em->text.data + s->value;
    size_t n = s->size;
    for (size_t i = 0; i < n; i++) {
        if (b[i] == 0x53) return 1;                 /* push %rbx */
        if (i + 1 < n && b[i] == 0x41 && (b[i + 1] == 0x54 || b[i + 1] == 0x55))
            return 1;                               /* push %r12 / %r13 */
    }
    return 0;
}

static void test_o0_leaf_no_dead_cs_push(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int sum6(int a, int b, int c, int d, int e, int f) {"
        "  return a + b + c + d + e + f;"
        "}"
        "int main() { return sum6(1, 2, 3, 4, 5, 6); }");
    T_ASSERT(find_sym(&em, "sum6") != NULL);
    T_ASSERT(!fn_text_has_push_cs(&em, "sum6"));
    emit_module_free(&em);
}

static int fn_bytes(const EmitModule *em, const char *name,
                    const unsigned char **out, size_t *n) {
    const EmitSymbol *s = find_sym(em, name);
    if (!s || s->value + s->size > em->text.len) return 0;
    *out = (const unsigned char *)em->text.data + s->value;
    *n = s->size;
    return 1;
}

/* and $imm8, r32  →  83 /4 ib  (optional 0x40/0x41 REX). */
static int fn_has_and_imm8(const EmitModule *em, const char *name, unsigned imm) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 2 < n; i++) {
        size_t k = i;
        if (b[k] == 0x40 || b[k] == 0x41) k++;
        if (k + 2 < n && b[k] == 0x83 && ((b[k + 1] & 0xF8) == 0xE0)
            && b[k + 2] == (unsigned char)imm)
            return 1;
    }
    return 0;
}

/* or $imm8, r32  →  83 /1 ib */
static int fn_has_or_imm8(const EmitModule *em, const char *name, unsigned imm) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 2 < n; i++) {
        size_t k = i;
        if (b[k] == 0x40 || b[k] == 0x41) k++;
        if (k + 2 < n && b[k] == 0x83 && ((b[k + 1] & 0xF8) == 0xC8)
            && b[k + 2] == (unsigned char)imm)
            return 1;
    }
    return 0;
}

/* mov $imm32, r32  →  B8+r ibyte[4]  (optional REX.B). */
static int fn_has_mov_imm32(const EmitModule *em, const char *name, unsigned imm) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 4 < n; i++) {
        size_t k = i;
        if (b[k] == 0x41) k++;
        if (k + 4 < n && (b[k] & 0xF8) == 0xB8) {
            unsigned v = (unsigned)b[k+1] | ((unsigned)b[k+2] << 8)
                       | ((unsigned)b[k+3] << 16) | ((unsigned)b[k+4] << 24);
            if (v == imm) return 1;
        }
    }
    return 0;
}

/* pop %rbp; ret  —  5D C3.  Shared epilogue should emit this once. */
static int fn_count_pop_rbp_ret(const EmitModule *em, const char *name) {
    const unsigned char *b; size_t n;
    int c = 0;
    if (!fn_bytes(em, name, &b, &n)) return -1;
    for (size_t i = 0; i + 1 < n; i++)
        if (b[i] == 0x5D && b[i + 1] == 0xC3) c++;
    return c;
}

static void test_o0_and_imm(void) {
    EmitModule em = compile_o0(
        "package main;"
        "unsigned mask15(unsigned x) { return x & 15u; }"
        "int main() { return (int)mask15(1); }");
    T_ASSERT(find_sym(&em, "mask15") != NULL);
    T_ASSERT(fn_has_and_imm8(&em, "mask15", 15));
    T_ASSERT(!fn_has_mov_imm32(&em, "mask15", 15));
    emit_module_free(&em);
}

static void test_o0_or_imm_via_trunc(void) {
    EmitModule em = compile_o0(
        "package main;"
        "unsigned or1(unsigned x) { return x | 1u; }"
        "int main() { return (int)or1(2); }");
    T_ASSERT(find_sym(&em, "or1") != NULL);
    T_ASSERT(fn_has_or_imm8(&em, "or1", 1));
    T_ASSERT(!fn_has_mov_imm32(&em, "or1", 1));
    emit_module_free(&em);
}

/* orl $imm32, r/m32  →  81 /1 id  (optional REX, then ModRM/SIB/disp). */
static int fn_has_or_imm32_mem(const EmitModule *em, const char *name,
                               unsigned imm) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 5 < n; i++) {
        size_t k = i;
        if (b[k] == 0x40 || b[k] == 0x41 || b[k] == 0x42 || b[k] == 0x43) k++;
        if (k + 5 >= n || b[k] != 0x81) continue;
        unsigned char modrm = b[k + 1];
        if ((modrm & 0x38) != 0x08) continue; /* /1 */
        size_t p = k + 2;
        int mod = (modrm >> 6) & 3;
        int rm = modrm & 7;
        if (mod == 3) continue; /* register dest, not mem */
        if (rm == 4) p++; /* SIB */
        if (mod == 1) p += 1;
        else if (mod == 2 || (mod == 0 && rm == 5) || (mod == 0 && rm == 4)) {
            /* disp32, or SIB with base=5 handled as disp32 when mod=0.
             * Conservatively accept a 4-byte follow if present. */
            if (mod == 2 || (mod == 0 && ((rm == 5) || (rm == 4))))
                p += 4;
        }
        if (p + 3 >= n) continue;
        unsigned v = (unsigned)b[p] | ((unsigned)b[p + 1] << 8)
                   | ((unsigned)b[p + 2] << 16) | ((unsigned)b[p + 3] << 24);
        if (v == imm) return 1;
    }
    return 0;
}

static void test_o0_or_imm_rmw(void) {
    /* TRUNC-hidden immediates used to skip CONST then fall back to
     * `or %reg, mem` with an undefined register (pr29797 / pr58943). */
    EmitModule em = compile_o0(
        "package main;"
        "unsigned g;"
        "unsigned or_local(unsigned x) { unsigned n = x; n |= 65536u; return n; }"
        "unsigned or_global(void) { g |= 128u; return g; }"
        "int main() { return (int)(or_local(1) + or_global()); }");
    T_ASSERT(find_sym(&em, "or_local") != NULL);
    T_ASSERT(find_sym(&em, "or_global") != NULL);
    T_ASSERT(fn_has_or_imm32_mem(&em, "or_local", 65536u));
    T_ASSERT(fn_has_or_imm32_mem(&em, "or_global", 128u));
    T_ASSERT(!fn_has_mov_imm32(&em, "or_local", 65536u));
    T_ASSERT(!fn_has_mov_imm32(&em, "or_global", 128u));
    emit_module_free(&em);
}

static void test_o0_shared_epilogue(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int sign3(int x) {"
        "  if (x > 0) return x + 1;"
        "  if (x < 0) return x - 1;"
        "  return 0;"
        "}"
        "int main() { return sign3(2) + sign3(-3) + sign3(0); }");
    T_ASSERT(find_sym(&em, "sign3") != NULL);
    T_ASSERT(fn_count_pop_rbp_ret(&em, "sign3") == 1);
    emit_module_free(&em);
}



/* lea r, disp8(%rbp)  →  (optional REX) 8D /r with mod=01 rm=101. */
static int fn_has_lea_rbp_disp8(const EmitModule *em, const char *name) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 2 < n; i++) {
        size_t k = i;
        if (b[k] == 0x40 || b[k] == 0x41 || b[k] == 0x48 || b[k] == 0x49
            || b[k] == 0x4C || b[k] == 0x4D)
            k++;
        if (k + 2 < n && b[k] == 0x8D && (b[k + 1] & 0xC7) == 0x45)
            return 1;
    }
    return 0;
}

static void test_o0_vec_folded_no_dest_lea(void) {
    /* Folded vadd/vxor of frame vectors must not lea the dest just to
     * store with movups [rbp+off] (vec c=a+b; a=c^b). */
    EmitModule em = compile_o0(
        "package main;"
        "typedef int V __attribute__((vector_size(16)));"
        "V addxor(void) {"
        "  V a, b, c;"
        "  a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;"
        "  b[0] = 5; b[1] = 6; b[2] = 7; b[3] = 8;"
        "  c = a + b;"
        "  a = c ^ b;"
        "  return a;"
        "}"
        "int main() { V r = addxor(); return r[0]; }");
    T_ASSERT(find_sym(&em, "addxor") != NULL);
    T_ASSERT(!fn_has_lea_rbp_disp8(&em, "addxor"));
    emit_module_free(&em);
}


/* movaps xmm, mem / mem, xmm  →  (REX) 0F 28/29 /r, r/m not 11. */
static int fn_has_movaps_mem(const EmitModule *em, const char *name) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 2 < n; i++) {
        size_t k = i;
        if (b[k] >= 0x40 && b[k] <= 0x4F) k++;
        if (k + 2 < n && b[k] == 0x0F && (b[k + 1] == 0x28 || b[k + 1] == 0x29)
            && (b[k + 2] & 0xC0) != 0xC0)
            return 1;
    }
    return 0;
}

static int fn_has_movups_mem(const EmitModule *em, const char *name) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 2 < n; i++) {
        size_t k = i;
        if (b[k] >= 0x40 && b[k] <= 0x4F) k++;
        if (k + 2 < n && b[k] == 0x0F && (b[k + 1] == 0x10 || b[k + 1] == 0x11)
            && (b[k + 2] & 0xC0) != 0xC0)
            return 1;
    }
    return 0;
}

/* pxor xmm, mem  →  66 (REX) 0F EF /r with r/m not 11 (not reg-reg). */
static int fn_has_pxor_mem(const EmitModule *em, const char *name) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 3 < n; i++) {
        size_t k = i;
        if (b[k] != 0x66) continue;
        k++;
        if (b[k] >= 0x40 && b[k] <= 0x4F) k++;
        if (k + 2 < n && b[k] == 0x0F && b[k + 1] == 0xEF
            && (b[k + 2] & 0xC0) != 0xC0)
            return 1;
    }
    return 0;
}

static void test_o0_vec_aligned_movaps(void) {
    EmitModule em = compile_o0(
        "package main;"
        "typedef int V __attribute__((vector_size(16)));"
        "V addxor(void) {"
        "  V a, b, c;"
        "  a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;"
        "  b[0] = 5; b[1] = 6; b[2] = 7; b[3] = 8;"
        "  c = a + b;"
        "  a = c ^ b;"
        "  return a;"
        "}"
        "int main() { V r = addxor(); return r[0]; }");
    T_ASSERT(find_sym(&em, "addxor") != NULL);
    T_ASSERT(fn_has_movaps_mem(&em, "addxor"));
    T_ASSERT(fn_has_pxor_mem(&em, "addxor"));
    /* The return copy of a 16-aligned local may still use movups; the
     * folded vadd/vxor path must not. */
    emit_module_free(&em);
}

static void test_o0_vec_unaligned_ptr_movups(void) {
    /* Pointer operands are not proved 16-aligned; must not use movaps. */
    EmitModule em = compile_o0(
        "package main;"
        "typedef int V __attribute__((vector_size(16)));"
        "void addxor_ptr(V *p, V *q) {"
        "  V t = *p + *q;"
        "  *p = t ^ *q;"
        "}"
        "int main() { return 0; }");
    T_ASSERT(find_sym(&em, "addxor_ptr") != NULL);
    T_ASSERT(fn_has_movups_mem(&em, "addxor_ptr"));
    T_ASSERT(!fn_has_movaps_mem(&em, "addxor_ptr"));
    emit_module_free(&em);
}


/* REX.W 89 /r to a memory operand with base %rbp: 8-byte spill of a GP
 * SSA temp.  Named O0 i32 locals use 32-bit stores (no REX.W). */
static int fn_has_movq_rbp(const EmitModule *em, const char *name) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 2 < n; i++) {
        unsigned char rex = b[i];
        if ((rex & 0xF8) != 0x48) continue; /* REX with W=1 */
        if (b[i + 1] != 0x89) continue;
        unsigned char modrm = b[i + 2];
        int mod = (modrm >> 6) & 3, rm = modrm & 7;
        if (mod == 3) continue;
        if (rm == 5) return 1;                 /* [rbp+disp] */
        if (rm == 4 && i + 3 < n) {
            unsigned char sib = b[i + 3];
            if ((sib & 7) == 5) return 1;      /* SIB base = rbp */
        }
    }
    return 0;
}

/* a[i&3] += (i&7) must keep SIB addressing without spilling the i&3 SSA
 * temp (not a named O0 local) to an 8-byte frame slot. */

/* sub $imm, %rsp: REX.W 83 /5 ib, or REX.W 81 /5 id. */
static int fn_has_sub_rsp_imm(const EmitModule *em, const char *name, int imm) {
    const unsigned char *b; size_t n;
    if (!fn_bytes(em, name, &b, &n)) return 0;
    for (size_t i = 0; i + 3 < n; i++) {
        if (b[i] != 0x48) continue;
        if (b[i + 1] == 0x83 && b[i + 2] == 0xEC
            && b[i + 3] == (unsigned char)imm)
            return 1;
        if (b[i + 1] == 0x81 && b[i + 2] == 0xEC && i + 6 < n) {
            int v = (int)(int32_t)((unsigned)b[i + 3]
                | ((unsigned)b[i + 4] << 8)
                | ((unsigned)b[i + 5] << 16)
                | ((unsigned)b[i + 6] << 24));
            if (v == imm) return 1;
        }
    }
    return 0;
}

static void test_o0_masked_index_no_spill(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int acc_mask(void) {"
        "  int a[4]; int i; int s;"
        "  for (i = 0; i < 4; i++) a[i] = i + 1;"
        "  for (i = 0; i < 16; i++) a[i & 3] += (i & 7);"
        "  s = 0;"
        "  for (i = 0; i < 4; i++) s += a[i];"
        "  return s;"
        "}"
        "int main() { return acc_mask(); }");
    T_ASSERT(find_sym(&em, "acc_mask") != NULL);
    T_ASSERT(text_has_sib_rbp_scale4(&em) || text_has_sib(&em, 0x0D));
    T_ASSERT(fn_has_and_imm8(&em, "acc_mask", 3));
    T_ASSERT(!fn_has_movq_rbp(&em, "acc_mask"));
    emit_module_free(&em);
}

/* When (i&3) is also stored to a named local, that value must still be
 * materialized; the index form of a[k] / a[i&3] stays a SIB. */
static void test_o0_masked_index_named_keep(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int with_k(void) {"
        "  int a[4]; int i; int k; int s;"
        "  for (i = 0; i < 4; i++) a[i] = 0;"
        "  s = 0;"
        "  for (i = 0; i < 8; i++) {"
        "    k = i & 3;"
        "    a[k] += k;"
        "    a[i & 3] += 1;"
        "    s += k;"
        "  }"
        "  return s + a[0] + a[1] + a[2] + a[3];"
        "}"
        "int main() { return with_k(); }");
    T_ASSERT(find_sym(&em, "with_k") != NULL);
    T_ASSERT(text_has_sib_rbp_scale4(&em) || text_has_sib(&em, 0x0D));
    T_ASSERT(fn_has_and_imm8(&em, "with_k", 3));
    emit_module_free(&em);
}

/* Same remat must hold under the register pressure of the vec benchmark
 * shape: packed vadd/vxor plus b[i&3] += (i&15).  Without remat, the
 * zext(i&3) SSA was spilled with movq to the frame. */
static void test_o0_vec_masked_index_no_spill(void) {
    EmitModule em = compile_o0(
        "package main;"
        "typedef int V __attribute__((vector_size(16)));"
        "V addxor_idx(void) {"
        "  V a, b, c; int i;"
        "  a[0] = 1; a[1] = 2; a[2] = 3; a[3] = 4;"
        "  b[0] = 5; b[1] = 6; b[2] = 7; b[3] = 8;"
        "  for (i = 0; i < 16; i++) {"
        "    c = a + b;"
        "    a = c ^ b;"
        "    b[i & 3] += (i & 15);"
        "  }"
        "  return a;"
        "}"
        "int main() { V r = addxor_idx(); return r[0]; }");
    T_ASSERT(find_sym(&em, "addxor_idx") != NULL);
    T_ASSERT(text_has_sib_rbp_scale4(&em) || text_has_sib(&em, 0x0D));
    T_ASSERT(fn_has_and_imm8(&em, "addxor_idx", 3));
    T_ASSERT(!fn_has_movq_rbp(&em, "addxor_idx"));
    /* Pinned IR_ALLOCA dests must not reserve GP spill slots.  Three
     * 16-byte vectors plus two i32s are 64 bytes of pinned area; with no
     * real SSA spill the frame is $0x40, not the previous $0x60. */
    T_ASSERT(fn_has_sub_rsp_imm(&em, "addxor_idx", 0x40));
    T_ASSERT(!fn_has_sub_rsp_imm(&em, "addxor_idx", 0x60));
    emit_module_free(&em);
}

/* A leaf of only 4-byte pinned locals must not grow a phantom spill
 * area just because the ALLOCA SSA values were in the GP class. */
static void test_o0_scalar_locals_compact_frame(void) {
    EmitModule em = compile_o0(
        "package main;"
        "int sum4(void) {"
        "  int a, b, c, d;"
        "  a = 1; b = 2; c = 3; d = 4;"
        "  return a + b + c + d;"
        "}"
        "int main() { return sum4(); }");
    T_ASSERT(find_sym(&em, "sum4") != NULL);
    /* Four i32s rounded to 8-byte slots = 32 bytes; no leftover $0x20+
     * spill reservation from the alloca SSAs themselves. */
    T_ASSERT(fn_has_sub_rsp_imm(&em, "sum4", 0x20));
    emit_module_free(&em);
}


/* A 16-byte INTEGER-class struct returned in rax/rdx and stored into
 * distinct destinations must not pin a dummy 8-byte "keep" alloca per
 * call site.  Those slots were store-only (never reloaded) and pushed
 * named locals past disp8.  IR_CALL.a is live after return instead. */
static void test_o0_struct_ret_no_keep_slot(void) {
    EmitModule em = compile_o0(
        "package main;"
        "typedef struct { int a; int b; long long c; } S;"
        "S mk(long long x) { S s; s.a = (int)x; s.b = (int)x + 1; s.c = x; return s; }"
        "S g[8];"
        "void fill8(void) {"
        "  g[0] = mk(0); g[1] = mk(1); g[2] = mk(2); g[3] = mk(3);"
        "  g[4] = mk(4); g[5] = mk(5); g[6] = mk(6); g[7] = mk(7);"
        "}"
        "int main() { fill8(); return (int)g[7].c; }");
    T_ASSERT(find_sym(&em, "fill8") != NULL);
    /* Eight call sites used to reserve 64 bytes of keep slots, so the
     * frame was around $0x50.  With no dummy keeps and no named locals
     * besides possible spills, the prologue must stay well under that. */
    T_ASSERT(!fn_has_sub_rsp_imm(&em, "fill8", 0x50));
    T_ASSERT(!fn_has_sub_rsp_imm(&em, "fill8", 0x60));
    T_ASSERT(!fn_has_sub_rsp_imm(&em, "fill8", 0x70));
    T_ASSERT(!fn_has_sub_rsp_imm(&em, "fill8", 0x80));
    emit_module_free(&em);
}

/* Named O0 local destination must still receive the rax/rdx store.
 * The frame holds the 16-byte local, not an extra keep slot beside it. */
static void test_o0_struct_ret_named_local(void) {
    EmitModule em = compile_o0(
        "package main;"
        "typedef struct { int a; int b; long long c; } S;"
        "S mk(long long x) { S s; s.a = (int)x; s.b = 0; s.c = x; return s; }"
        "long long use_tmp(void) {"
        "  S t;"
        "  t = mk(42);"
        "  return t.c;"
        "}"
        "int main() { return (int)use_tmp(); }");
    T_ASSERT(find_sym(&em, "use_tmp") != NULL);
    /* Named 16-byte t must sit in the frame (rax/rdx stored to [rbp+off]).
     * A leftover even-rounded GP spill area may still make the prologue
     * $0x20 rather than $0x10; a dummy keep slot would push it to $0x30. */
    T_ASSERT(fn_has_sub_rsp_imm(&em, "use_tmp", 0x10)
          || fn_has_sub_rsp_imm(&em, "use_tmp", 0x20));
    T_ASSERT(!fn_has_sub_rsp_imm(&em, "use_tmp", 0x30));
    emit_module_free(&em);
}

/* ---- main ---- */


int main(void) {
    /* These cases pin the x86-64 ELF backend; a parallel arm64 suite will
     * select the arm64-macos target once that backend exists. */
    target_set_current(target_x86_64_linux());
    test_return_zero();
    test_return_42();
    test_return_255();
    test_prologue_present();
    test_var_codegen();
    test_arith_codegen_longer();
    test_if_codegen_has_main();
    test_multi_function_symbols();
    test_bitfield_codegen();
    test_sib_global_index();
    test_sib_local_index();
    test_vector16_uses_movups();
    test_constructor_init_array();
    test_destructor_fini_array();
    test_constructor_priority_slots();
    test_rol8_encoding();
    test_builtin_return_codegen();
    test_o0_leaf_no_dead_cs_push();
    test_o0_and_imm();
    test_o0_or_imm_via_trunc();
    test_o0_or_imm_rmw();
    test_o0_shared_epilogue();
    test_o0_vec_folded_no_dest_lea();
    test_o0_vec_aligned_movaps();
    test_o0_vec_unaligned_ptr_movups();
    test_o0_masked_index_no_spill();
    test_o0_masked_index_named_keep();
    test_o0_vec_masked_index_no_spill();
    test_o0_scalar_locals_compact_frame();
    test_o0_struct_ret_no_keep_slot();
    test_o0_struct_ret_named_local();
    return t_finalize();
}
