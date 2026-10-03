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

/* Compile and run.  extra_argv are appended after the program name;
 * when out != NULL child stdout is piped into out[0..outcap-1]. */
static int compile_and_run_ex(const char *source, char *const extra_argv[],
                              char *out, size_t outcap) {
    TokenArray tokens;
    token_array_init(&tokens);
    if (lex(source, "<case>", &tokens) != FAKECC_OK) return -1001;
    TranslationUnit tu; tu_init(&tu);
    if (parse(&tokens, &tu) != FAKECC_OK) return -1002;
    if (sema_check(&tu, 0) != FAKECC_OK || sema_has_errors()) return -1003;

    IRModule ir; ir_module_init(&ir);
    /* CG64_OPT=1 runs the mem2reg/-O1 pipeline (T9). Default stays -O0. */
    const char *opt_env = getenv("CG64_OPT");
    int opt_level = opt_env ? atoi(opt_env) : 0;
    if (ir_generate(&tu, &ir, opt_level == 0) != FAKECC_OK) return -1004;
    opt(&ir, opt_level, 0);

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

    int pipefd[2] = {-1, -1};
    if (out && pipe(pipefd) != 0) return -1009;

    pid_t pid = fork();
    if (pid < 0) return -1007;
    if (pid == 0) {
        if (out) {
            dup2(pipefd[1], 1);
            close(pipefd[0]); close(pipefd[1]);
        }
        /* Build argv: program name then extras (NULL terminated). */
        int n = 0;
        while (extra_argv && extra_argv[n]) n++;
        char **av = malloc((size_t)(n + 2) * sizeof(char *));
        av[0] = (char *)path;
        for (int i = 0; i < n; i++) av[i + 1] = extra_argv[i];
        av[n + 1] = NULL;
        execv(path, av);
        _exit(127);
    }
    if (out) {
        close(pipefd[1]);
        size_t total = 0;
        for (;;) {
            ssize_t r = read(pipefd[0], out + total,
                             outcap - 1 - total < 4096 ? outcap - 1 - total : 4096);
            if (r <= 0) break;
            total += (size_t)r;
            if (total >= outcap - 1) break;
        }
        out[total] = 0;
        close(pipefd[0]);
    }
    int st;
    waitpid(pid, &st, 0);
    return WIFEXITED(st) ? WEXITSTATUS(st) : -1008;
}

static int compile_and_run(const char *source) {
    return compile_and_run_ex(source, NULL, NULL, 0);
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

/* T7: process entry ABI — argc/argv/envp handed through the stub. */
static void test_entry_abi(void) {
    const char *src =
        "package main;\n"
        "int main(int argc, char **argv) {\n"
        " if (argc != 3) return 1;\n"
        " if (argv[1][0]!='a'||argv[1][1]!='b'||argv[1][2]!=0) return 2;\n"
        " if (argv[2][0]!='z'||argv[2][1]!=0) return 3;\n"
        " return 0; }\n";
    char *av[] = { "ab", "z", NULL };
    T_ASSERT_EQ_INT(compile_and_run_ex(src, av, NULL, 0), 0);

    /* TR-7.2: envp is walkable; locate PATH and write its value to stdout
     * via the raw Darwin write syscall (freestanding, no libc yet). */
    const char *env_src =
        "package main;\n"
        "int main(int argc, char **argv, char **envp) {\n"
        " int i = 0;\n"
        " while (envp[i]) {\n"
        "  char *e = envp[i];\n"
        "  if (e[0]=='P'&&e[1]=='A'&&e[2]=='T'&&e[3]=='H'&&e[4]=='=') {\n"
        "   char *v = e+5; int len=0; while (v[len]) len++;\n"
        "   __syscall(4, 1, v, len);\n"
        "   return 0; }\n"
        "  i++; }\n"
        " return 5; }\n";
    char out[4096];
    int rc = compile_and_run_ex(env_src, NULL, out, sizeof out);
    T_ASSERT_EQ_INT(rc, 0);
    const char *path = getenv("PATH");
    if (!path) path = "";
    if (strcmp(out, path) != 0) {
        fprintf(stderr, "  envp PATH mismatch: got '%s' want '%s'\n", out, path);
        T_ASSERT_EQ_INT(1, 0);
    }
    T_ASSERT(strlen(out) > 0);
}

/* T7: constructors/destructors run from the entry stub, ordered by
 * priority.  Observed through raw exit syscalls (globals arrive in T8). */
static void test_ctor_dtor(void) {
    expect("ctor_priority_order",
        "package main;\n"
        "__attribute__((constructor(200))) void late(void){ __syscall(1,21); }\n"
        "__attribute__((constructor(100))) void early(void){ __syscall(1,11); }\n"
        "int main(){ return 0; }", 11);
    expect("ctor_default_prio",
        "package main;\n"
        "__attribute__((constructor)) void c(void){ __syscall(1,77); }\n"
        "int main(){ return 0; }", 77);
    /* Destructors walk the priority list backwards (highest first), and
     * run AFTER main (main's own return 9 must not be observed). */
    expect("dtor_reverse_order",
        "package main;\n"
        "__attribute__((destructor(100))) void a(void){ __syscall(1,31); }\n"
        "__attribute__((destructor(200))) void b(void){ __syscall(1,32); }\n"
        "int main(){ return 9; }", 32);
    expect("no_ctor_main_runs",
        "package main;\nint main(){ return 42; }", 42);
}

/* T7: mixed-width parameters beyond x0..x7 travel on the stack under
 * the AAPCS sign/zero-extension rules. */
static void test_mixed_width_stack(void) {
    expect("signed_narrow_stack",
        "package main;\n"
        "long f(long a,long b,long c,long d,long e,long g,long h,long i,\n"
        "       char j, short k, int l, long m){\n"
        " return a+b+c+d+e+g+h+i+j+k+l+m; }\n"
        "int main(){ return (int)(f(1,2,3,4,5,6,7,8,-9,-10,-11,-12)%256); }",
        250);
    expect("unsigned_narrow_stack",
        "package main;\n"
        "long f(long a,long b,long c,long d,long e,long g,long h,long i,\n"
        "       unsigned char j, unsigned short k, unsigned int l,\n"
        "       unsigned long m) {\n"
        " return a+b+c+d+e+g+h+i+j+k+l+m; }\n"
        "int main(){ return (int)(f(1,2,3,4,5,6,7,8,9,10,11,12)%256); }",
        (int)((36 + 42) % 256));
    expect("all_mixed_12",
        "package main;\n"
        "long f(char a, short b, int c, long d, unsigned char e,\n"
        "       unsigned short g, unsigned int h, unsigned long i,\n"
        "       char j, short k, int l, long m){\n"
        " return a+b+c+d+e+g+h+i+j+k+l+m; }\n"
        "int main(){ return (int)(f(-1,-2,-3,-4,5,6,7,8,-9,-10,-11,-12)%256); }",
        230);
}

/* T8: globals, strings, aggregates, and PIE pointer initializers. */
static void test_globals_and_aggregates(void) {
    expect("global_bss_const",
        "package main;\n"
        "int g = 40;\n"
        "int z;\n"
        "const int c = 2;\n"
        "int main(){ z = 1; return g + z + c - 1; }", 42);
    expect("string_literal",
        "package main;\n"
        "int main(){ char *p = \"hi\";\n"
        " return p[0]=='h' && p[1]=='i' && p[2]==0; }", 1);
    expect("static_local",
        "package main;\n"
        "int bump(void){ static int n = 4; n = n + 1; return n; }\n"
        "int main(){ return bump() + bump(); }", 11);
    expect("struct_copy_fields",
        "package main;\n"
        "struct S { int a; char b; int c; };\n"
        "int main(){ struct S x; x.a=1; x.b=2; x.c=3;\n"
        " struct S y; y = x; return y.a + y.b + y.c; }", 6);
    expect("array_2d",
        "package main;\n"
        "int a[3][2];\n"
        "int main(){ a[1][1] = 7; a[2][0] = 5;\n"
        " int *p = &a[0][0]; return a[1][1] + *(p+4); }", 12);
    expect("bitfield",
        "package main;\n"
        "struct B { int x:3; unsigned y:5; int z:10; };\n"
        "int main(){ struct B b; b.x=-1; b.y=17; b.z=100;\n"
        " return (b.x==-1) + (b.y==17) + (b.z==100); }", 3);
    expect("union_le",
        "package main;\n"
        "union U { int i; char c[4]; };\n"
        "int main(){ union U u; u.i = 0x01020304; return u.c[0]; }", 4);
    expect("ptr_cmp",
        "package main;\n"
        "int g = 10;\n"
        "int main(){ int *p = &g; int *q = &g;\n"
        " return (*p == 10) && (p == q); }", 1);
    /* >64-byte assignment lowers to memcpy; the backend inlines it. */
    expect("struct_copy_memcpy",
        "package main;\n"
        "struct Big { char b[80]; };\n"
        "int main(){\n"
        " struct Big a; struct Big c; int i; int s;\n"
        " i = 0; while (i < 80) { a.b[i] = i; i = i + 1; }\n"
        " c = a;\n"
        " i = 0; s = 0; while (i < 80) { s = s + c.b[i]; i = i + 1; }\n"
        " return s == 3160; }", 1);
    expect("builtin_memset",
        "package main;\n"
        "int main(){\n"
        " char b[16]; int i; int s;\n"
        " i = 0; while (i < 16) { b[i] = 1; i = i + 1; }\n"
        " __builtin_memset(b, 7, 16);\n"
        " i = 0; s = 0; while (i < 16) { s = s + b[i]; i = i + 1; }\n"
        " return s; }", 112);
    expect("global_fnptr_and_ptr",
        "package main;\n"
        "int add(int a, int b){ return a + b; }\n"
        "int sub(int a, int b){ return a - b; }\n"
        "int (*ft[2])(int, int) = { add, sub };\n"
        "int g = 40;\n"
        "int arr[4] = { 1, 2, 3, 4 };\n"
        "int *p = &arr[2];\n"
        "char *s = \"ok\";\n"
        "int main(){ return ft[0](1, 1) + ft[1](10, 3) + *p + (s[0]=='o'); }",
        2 + 7 + 3 + 1);
}

/* TR-8.2: the same image, loaded twice, yields the same result, and
 * dyld reports a chained rebase.  `dyld_info -fixups` also walks the
 * symbol table to name targets; that table arrives in T14, so the
 * chain dump (`-fixup_chains`) is the check that works today. */
static void test_pie_rebase(void) {
    const char *src =
        "package main;\n"
        "int g = 7;\n"
        "int *p = &g;\n"
        "int main(){ return *p; }\n";
    T_ASSERT_EQ_INT(compile_and_run(src), 7);
    FILE *fp = popen("/usr/bin/dyld_info -fixup_chains /tmp/fakecc_cg64_native_test", "r");
    T_ASSERT(fp != NULL);
    char buf[2048];
    size_t n = fread(buf, 1, sizeof buf - 1, fp);
    buf[n] = 0;
    pclose(fp);
    if (!strstr(buf, "DYLD_CHAINED_PTR_64_OFFSET") || !strstr(buf, "start[")) {
        fprintf(stderr, "  dyld_info missing chained rebase:\n%s\n", buf);
        T_ASSERT(0);
    }
    pid_t pid = fork();
    T_ASSERT(pid >= 0);
    if (pid == 0) {
        execl("/tmp/fakecc_cg64_native_test", "t", (char *)NULL);
        _exit(127);
    }
    int st;
    waitpid(pid, &st, 0);
    T_ASSERT(WIFEXITED(st) && WEXITSTATUS(st) == 7);
}

/* T7: deep recursion stress (many frames + callee-saved pressure). */
static void test_recursion_deep(void) {
    expect("deep_recursion",
        "package main;\n"
        "int rec(int n){ return n ? 1 + rec(n-1) : 0; }\n"
        "int main(){ int v = rec(10000); return v==10000 ? 0 : 1; }", 0);
}

int main(void) {
    target_set_current(target_arm64_macos());
    test_divmod_edgecases();
    test_shift_edges();
    test_switch_shapes();
    test_trunc_ext();
    test_deep_calls();
    test_entry_abi();
    test_ctor_dtor();
    test_mixed_width_stack();
    test_recursion_deep();
    test_globals_and_aggregates();
    test_pie_rebase();
    return t_finalize();
}

#else

int main(void) { return t_finalize(); }  /* non-Apple-Silicon: no-op */

#endif
