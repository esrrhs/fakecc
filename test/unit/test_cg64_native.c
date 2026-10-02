/*
 * End-to-end native tests for the arm64 Mach-O backend: compile a small C
 * source through the whole pipeline (parse → IR -O0 → arm64 codegen →
 * Mach-O + ad-hoc codesign), execute it, and check its exit status.
 *
 * Enabled only on Apple Silicon hosts, where the produced images run
 * directly.  Freestanding programs only (no libc/globals beyond locals).
 */
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/opt.h"
#include "fakecc/emit.h"
#include "fakecc/codegen.h"
#include "fakecc/macho.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

#if defined(__APPLE__) && defined(__aarch64__)

static int compile_and_run(const char *source) {
    TokenArray tokens;
    token_array_init(&tokens);
    if (lex(source, "<case>", &tokens) != FAKECC_OK) return -1001;
    TranslationUnit tu; tu_init(&tu);
    if (parse(&tokens, &tu) != FAKECC_OK) return -1002;
    if (sema_check(&tu, 0) != FAKECC_OK || sema_has_errors()) return -1003;

    IRModule ir; ir_module_init(&ir);
    if (ir_generate(&tu, &ir, 1) != FAKECC_OK) return -1004;
    opt(&ir, 0, 0);

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);

    const char *path = "/tmp/fakecc_cg64_native_test";
    if (macho_write_exec(&em, macho_text_offset(), path) != 0)
        return -1005;
    emit_module_free(&em);
    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&tokens);
    if (macho_codesign(path) != 0) return -1006;

    pid_t pid = fork();
    if (pid < 0) return -1007;
    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(127);
    }
    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1008;
}

static void expect(const char *name, const char *src, int want) {
    int got = compile_and_run(src);
    if (got != want)
        fprintf(stderr, "  %s: got %d want %d\n", name, got, want);
    T_ASSERT_EQ_INT(got, want);
}

static void test_divmod_edgecases(void) {
    /* INT_MIN / -1 and % -1: A64 SDIV saturates to INT_MIN (x86 IDIV
     * overflows); the backend must produce the same C-observable result. */
    expect("intmin_div",
        "package main;\nint main(){\n"
        " int q = (-2147483647 - 1) / -1;\n"
        " int r = (-2147483647 - 1) % -1;\n"
        " return (q == -2147483647-1) && (r == 0); }", 1);
    expect("div_mod_consistency",
        "package main;\nint main(){\n"
        " int a=-53,b=7; return a - (a/b)*b - a%b; }", 0);
    expect("unsigned_divmod",
        "package main;\nint main(){\n"
        " unsigned x=0xFFFFFFFFu, y=7u;\n"
        " return (int)(x/y) + (int)(0u - (x%y)*0); }", 613566756 & 0xff);
    expect("div_by_one",
        "package main;\nint main(){\n"
        " return (12345/1)*0 + (-99/-1) - 99; }", 0);
}

static void test_shift_edges(void) {
    /* Shifts by >= width are UB in C; both x86 (CL masked) and A64
     * (LSLV/LSRV masked) use count mod 32, so the observable result
     * matches the x86 backend. */
    expect("shl32_32",
        "package main;\nint main(){ unsigned x=1u; return (int)(x<<32); }", 1);
    expect("shr_var",
        "package main;\nint main(){\n"
        " unsigned s=0;\n for(unsigned i=0;i<40;i++) s += (1u<<i);\n"
        " return (int)s; }", 254); /* 0..31 wraps all bits (-1), 32..39=255 */
    expect("asr_sign",
        "package main;\nint main(){ int x=-16; return (x>>3) + 2; }", 0);
    expect("rol_large",
        "package main;\n"
        "unsigned rol(unsigned a,unsigned b){\n"
        " return (a<<(b&31)) | (a>>((32-b)&31)); }\n"
        "int main(){ return rol(0xA5A5A5A5u,36)==0x5A5A5A5Au; }", 1);
}

static void test_switch_shapes(void) {
    /* Dense and sparse compare-chain lowering. */
    expect("dense",
        "package main;\n"
        "int sw(int x){ switch(x){ case 0:return 1; case 1:return 2;\n"
        " case 2:return 4; case 3:return 8; case 4:return 16;\n"
        " default:return 0; } }\n"
        "int main(){ return sw(0)+sw(1)+sw(2)+sw(3)+sw(4)+sw(9); }", 31);
    expect("sparse",
        "package main;\n"
        "int sw(int x){ switch(x){ case -1000:return 1;\n"
        " case 0:return 2; case 1000:return 4; case 77777:return 8;\n"
        " default:return 16; } }\n"
        "int main(){ return sw(0)+sw(1000)+sw(5)+sw(-1000)+sw(77777); }", 31);
    expect("no_case",
        "package main;\n"
        "int main(){ int x=0; switch(3){ case 1: x=1; case 2: x=2; }\n"
        " return x; }", 0);
}

static void test_trunc_ext(void) {
    expect("sxtb",
        "package main;\nint main(){ char c=200; return (int)c == -56; }", 1);
    expect("zext",
        "package main;\nint main(){ unsigned char c=200;\n"
        " return (int)c == 200; }", 1);
    expect("narrow_store",
        "package main;\n"
        "int main(){ short a[2]; a[0]=-1; a[1]=300;\n"
        " return (int)a[0] + (int)a[1]; }", 43);
}

static void test_deep_calls(void) {
    expect("recursion",
        "package main;\n"
        "int fib(int n){ return n<2 ? n : fib(n-1)+fib(n-2); }\n"
        "int main(){ return fib(15)-610; }", 0);
    expect("stack_args",
        "package main;\n"
        "long f(long a,long b,long c,long d,long e,long g,long h,\n"
        "       long i,long j,long k,long l,long m){\n"
        " return a*1+b*2+c*3+d*4+e*5+g*6+h*7+i*8+j*9+k*10+l*11+m*12; }\n"
        "int main(){ long v=f(1,2,3,4,5,6,7,8,9,10,11,12);\n"
        " return (int)(v & 255); }",
        (int)((1*1+2*2+3*3+4*4+5*5+6*6+7*7+8*8+9*9+10*10+11*11+12*12) & 255));
    /* 32 parameters force every GP palette color (24 callee/caller
     * registers plus 24 stack slots).  Regression: codegen used to
     * re-index the allocator's NATIVE register codes through the palette
     * a second time, running into the adjacent SIMD table for colors
     * >= 20 and aliasing distinct values onto x0..x4. */
    expect("many_stack_args",
        "package main;\n"
        "int f(int a0,int a1,int a2,int a3,int a4,int a5,int a6,int a7,\n"
        "      int a8,int a9,int a10,int a11,int a12,int a13,int a14,int a15,\n"
        "      int a16,int a17,int a18,int a19,int a20,int a21,int a22,int a23,\n"
        "      int a24,int a25,int a26,int a27,int a28,int a29,int a30,int a31){\n"
        " int s=0;\n"
        " s=s+a0;s=s+a1;s=s+a2;s=s+a3;s=s+a4;s=s+a5;s=s+a6;s=s+a7;\n"
        " s=s+a8;s=s+a9;s=s+a10;s=s+a11;s=s+a12;s=s+a13;s=s+a14;s=s+a15;\n"
        " s=s+a16;s=s+a17;s=s+a18;s=s+a19;s=s+a20;s=s+a21;s=s+a22;s=s+a23;\n"
        " s=s+a24;s=s+a25;s=s+a26;s=s+a27;s=s+a28;s=s+a29;s=s+a30;s=s+a31;\n"
        " return s%256; }\n"
        "int main(){ return f(0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7,\n"
        "                   0,1,2,3,4,5,6,7,0,1,2,3,4,5,6,7); }",
        (4 * 28) % 256);
    /* Seven values stay live across a recursive call, so the allocator
     * homes them in x19..x25; under the old double mapping colors 19/20
     * physically landed on x0/x1 (clobbered by the call, not saved by
     * the prologue), corrupting the running sum. */
    expect("recursion_pressure",
        "package main;\n"
        "int sum(int n,int a,int b,int c,int d,int e,int g,int h){\n"
        " int s=a+b+c+d+e+g+h;\n"
        " return n==0 ? s : s + sum(n-1,a,b,c,d,e,g,h); }\n"
        "int main(){ return sum(10,1,2,3,4,5,6,7)%256; }",
        (11 * 28) % 256);
}

int main(void) {
    target_set_current(target_arm64_macos());
    test_divmod_edgecases();
    test_shift_edges();
    test_switch_shapes();
    test_trunc_ext();
    test_deep_calls();
    return t_finalize();
}

#else

int main(void) { return t_finalize(); }  /* non-Apple-Silicon: no-op */

#endif
