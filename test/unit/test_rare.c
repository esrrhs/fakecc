#include "fakecc/ast.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

static void expect_parse(const char *name, const char *src, int min_fns) {
    TranslationUnit tu = lex_parse(src);
    if ((int)tu.functions.len < min_fns) {
        fprintf(stderr, "rare parse %s: functions=%zu want>=%d\n",
                name, tu.functions.len, min_fns);
    }
    T_ASSERT((int)tu.functions.len >= min_fns);
    tu_free(&tu);
}

static void expect_ir(const char *name, const char *src) {
    IRModule ir = compile_to_ir(src);
    if (ir.functions.len < 1) {
        fprintf(stderr, "rare ir %s: no functions\n", name);
    }
    T_ASSERT(ir.functions.len >= 1);
    ir_module_free(&ir);
}

static void test_parse_rare(void) {
    expect_parse("sso_le",
        "package main; struct S { unsigned x; } "
        "__attribute__((scalar_storage_order(\"little-endian\"))); "
        "int main() { struct S s; s.x = 1; return (int)s.x; }", 1);

    expect_parse("c23_attr",
        "package main; [[gnu::unused]] int gx; "
        "int main() { [[gnu::unused]] int x; [[a([[b]])]] int y; return 0; }", 1);

    expect_parse("offsetof_index",
        "package main; struct Inner { int x; int y; }; "
        "struct S { struct Inner a[4]; int z; }; "
        "int main() { return (int)__builtin_offsetof(struct S, a[2].y); }", 1);

    expect_parse("typeof_assign",
        "package main; int main() { int x = 1; "
        "typeof(x = 2) y = 0; typeof(x += 1) z = 0; typeof(x++) w = 0; "
        "typeof((x, 1.0)) d = 0; typeof(&x) p = &x; return 0; }", 1);

    expect_parse("extra_ident",
        "package main; int foo(void), bar(int); int main() { return 0; }", 3);

    expect_parse("extra_star",
        "package main; int *p0(int), *p1(int); int main() { return 0; }", 3);

    expect_parse("extra_star_void",
        "package main; int *p2(void), *p3(void); int main() { return 0; }", 3);

    expect_parse("extra_star_ellipsis",
        "package main; int *p4(int, ...), *p5(int, ...); int main() { return 0; }", 3);

    expect_parse("extra_star_kr",
        "package main; int *k0(int), *k1(b, c); int main() { return 0; }", 3);

    expect_parse("extra_type",
        "package main; int f0(void), int f1(int); int main() { return 0; }", 3);

    expect_parse("extra_type_void",
        "package main; int f2(void), int f3(void); int main() { return 0; }", 3);

    expect_parse("extra_type_arr",
        "package main; int f4(void), int f5(int arr[]); int main() { return 0; }", 3);

    expect_parse("extra_type_ellipsis",
        "package main; int f6(void), int f7(int, ...); int main() { return 0; }", 3);

    expect_parse("extra_grow_cap",
        "package main; int a0(), a1(), a2(), a3(), a4(), a5(), a6(), a7(); "
        "int main() { return 0; }", 9);
}

static void test_ir_rare(void) {
    expect_ir("dfp_global",
        "package main; "
        "_Decimal64 g = 1.25DD; "
        "_Decimal64 n = -2.5DD; "
        "_Decimal64 fromi = 7; "
        "_Decimal64 sum = 1.10DD + 2.20DD; "
        "_Decimal64 diff = 5.00DD - 1.50DD; "
        "_Decimal64 prod = 2.00DD * 3.00DD; "
        "_Decimal64 quot = 9.00DD / 4.00DD; "
        "_Decimal128 q = 1.DL; "
        "_Decimal32 s = .5df; "
        "int main() { return (int)g; }");

    expect_ir("be_bitfield_init",
        "package main; struct B { unsigned a : 4; unsigned b : 4; } "
        "__attribute__((scalar_storage_order(\"big-endian\"))); "
        "struct B g = { 1, 2 }; "
        "int main() { return (int)g.a + (int)g.b; }");

    expect_ir("shuffle3",
        "package main; "
        "typedef int V __attribute__((vector_size(16))); "
        "int main() { "
        "  V a = {1,2,3,4}; V b = {5,6,7,8}; V m = {0,5,2,7}; "
        "  V r = __builtin_shuffle(a, b, m); "
        "  return r[0]; }");

    expect_ir("switch_range",
        "package main; int main() { int x = 3; "
        "switch (x) { case 1 ... 5: return 0; default: return 1; } }");

    expect_ir("switch_vla_goto",
        "package main; int main() { int n = 3; void *p = &&done; "
        "switch (n) { case 1 ... 4: { int a[n]; a[0] = 1; goto *p; } } "
        "return 1; done: return 0; }");

    expect_ir("builtin_return",
        "package main; int add1(int x) { return x + 1; } "
        "int wrap(int x) { "
        "  void *args = __builtin_apply_args(); "
        "  __builtin_return(__builtin_apply((void (*)())add1, args, 16)); "
        "} int main() { return wrap(41); }");

    expect_ir("complex_assign",
        "package main; int main() { "
        "_Complex double z; z = 3.0; "
        "_Complex float f; f = 1.0f; "
        "return (__real__ z != 3.0) || (__real__ f != 1.0f); }");

    expect_ir("rol32",
        "package main; unsigned rot(unsigned x, unsigned n) { "
        "  return (x << n) | (x >> (32u - n)); } "
        "int main() { return (int)rot(1u, 1u); }");
}

int main(void) {
    test_parse_rare();
    test_ir_rare();
    return t_finalize();
}
