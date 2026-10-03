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

/* Compile to the native image and leave it at the usual path. */
static int compile_image(const char *source) {
    TokenArray tokens;
    token_array_init(&tokens);
    if (lex(source, "<case>", &tokens) != FAKECC_OK) return -1;
    TranslationUnit tu; tu_init(&tu);
    if (parse(&tokens, &tu) != FAKECC_OK) return -1;
    if (sema_check(&tu, 0) != FAKECC_OK || sema_has_errors()) return -1;
    IRModule ir; ir_module_init(&ir);
    if (ir_generate(&tu, &ir, 1) != FAKECC_OK) return -1;
    opt(&ir, 0, 0);
    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);
    const char *path = "/tmp/fakecc_cg64_native_test";
    int rc = macho_write_exec(&em, macho_text_offset(), path);
    emit_module_free(&em);
    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&tokens);
    return rc;
}

static int read_macho_text(const char *path, unsigned char **out, size_t *outn) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;
    if (fseek(fp, 0, SEEK_END) != 0) { fclose(fp); return -1; }
    long sz = ftell(fp);
    if (sz < 32) { fclose(fp); return -1; }
    rewind(fp);
    unsigned char *data = malloc((size_t)sz);
    if (!data || fread(data, 1, (size_t)sz, fp) != (size_t)sz) {
        free(data); fclose(fp); return -1;
    }
    fclose(fp);
    uint32_t ncmds, off = 32;
    memcpy(&ncmds, data + 16, 4);
    for (uint32_t c = 0; c < ncmds; c++) {
        uint32_t cmd, cmdsize;
        memcpy(&cmd, data + off, 4);
        memcpy(&cmdsize, data + off + 4, 4);
        if (cmd == 0x19) { /* LC_SEGMENT_64 */
            uint32_t nsects;
            memcpy(&nsects, data + off + 64, 4);
            uint32_t so = off + 72;
            for (uint32_t s = 0; s < nsects; s++) {
                if (memcmp(data + so, "__text", 6) == 0) {
                    uint64_t size;
                    uint32_t fileoff;
                    memcpy(&size, data + so + 40, 8);
                    memcpy(&fileoff, data + so + 48, 4);
                    *outn = (size_t)size;
                    *out = malloc(*outn);
                    if (!*out) { free(data); return -1; }
                    memcpy(*out, data + fileoff, *outn);
                    free(data);
                    return 0;
                }
                so += 80;
            }
        }
        off += cmdsize;
    }
    free(data);
    return -1;
}

/* Function `which` in emission order.  0 is the LC_MAIN stub. */
static int nth_func(const unsigned char *text, size_t n, int which,
                    const unsigned char **start, size_t *len) {
    size_t i = 0, seen = 0, begin = 0;
    while (i + 4 <= n) {
        uint32_t w;
        memcpy(&w, text + i, 4);
        i += 4;
        if (w == 0xD65F03C0u) { /* ret */
            if ((int)seen == which) {
                *start = text + begin;
                *len = i - begin;
                return 0;
            }
            seen++;
            begin = i;
        }
    }
    return -1;
}

static void write_sym(FILE *fp, const char *name, const unsigned char *b, size_t n) {
    fprintf(fp, ".globl _%s\n.p2align 2\n_%s:\n", name, name);
    for (size_t i = 0; i < n; i++)
        fprintf(fp, ".byte %u\n", b[i]);
}

/* Point every BL in a fakecc caller at an `id` that will be appended
 * immediately after this function. */
static void retarget_bl(unsigned char *b, size_t n) {
    for (size_t i = 0; i + 4 <= n; i += 4) {
        uint32_t w;
        memcpy(&w, b + i, 4);
        if ((w >> 26) == 0x25) {
            int disp = (int)((n - i) / 4);
            w = 0x94000000u | ((uint32_t)disp & 0x3ffffffu);
            memcpy(b + i, &w, 4);
        }
    }
}

static int clang_exit(const char *csrc) {
    FILE *fp = fopen("/tmp/fakecc_abi_main.c", "w");
    if (!fp) return -1;
    fputs(csrc, fp);
    fclose(fp);
    int rc = system("clang -arch arm64 -O0 -o /tmp/fakecc_abi_cross "
                    "/tmp/fakecc_abi_main.c /tmp/fakecc_abi_fn.s >/tmp/fakecc_abi_clang.log 2>&1");
    if (rc != 0) return -2;
    rc = system("/tmp/fakecc_abi_cross");
    return WIFEXITED(rc) ? WEXITSTATUS(rc) : -3;
}

static void test_abi_cross(void) {
    /* clang calls a fakecc callee, and a fakecc caller calls a clang
     * callee.  Relocatable objects are T14, so the function bodies are
     * lifted out of the image and linked by clang. */
    const char *src =
        "package main;\n"
        "struct s32 { long a,b,c,d; };\n"
        "struct s32 id(struct s32 a){ a.d += a.a; return a; }\n"
        "int sum(void){ struct s32 a; a.a=1; a.b=2; a.c=3; a.d=4;\n"
        " struct s32 b; b=id(a); return (int)(b.a+b.b+b.c+b.d); }\n"
        "int main(){ return 0; }\n";
    T_ASSERT(compile_image(src) == 0);
    unsigned char *text = NULL; size_t tn = 0;
    T_ASSERT(read_macho_text("/tmp/fakecc_cg64_native_test", &text, &tn) == 0);
    const unsigned char *id = NULL, *sum = NULL;
    size_t idn = 0, sumn = 0;
    T_ASSERT(nth_func(text, tn, 1, &id, &idn) == 0);
    T_ASSERT(nth_func(text, tn, 2, &sum, &sumn) == 0);
    FILE *fp = fopen("/tmp/fakecc_abi_fn.s", "w");
    T_ASSERT(fp != NULL);
    fputs(".text\n", fp);
    write_sym(fp, "id", id, idn);
    fclose(fp);
    T_ASSERT_EQ_INT(clang_exit(
        "struct s32 { long a,b,c,d; };\n"
        "struct s32 id(struct s32 a);\n"
        "int main(void){ struct s32 a={1,2,3,4}; struct s32 b=id(a);\n"
        " return (int)(b.a+b.b+b.c+b.d); }\n"), 11);

    /* clang's id, fakecc's sum with its BL retargeted at that id. */
    fp = fopen("/tmp/clang_id.c", "w");
    T_ASSERT(fp != NULL);
    fputs("struct s32 { long a,b,c,d; };\n"
          "struct s32 id(struct s32 a){ a.d += a.a; return a; }\n", fp);
    fclose(fp);
    T_ASSERT(system("clang -arch arm64 -O0 -c /tmp/clang_id.c -o /tmp/clang_id.o") == 0);
    unsigned char *ct = NULL; size_t cn = 0;
    T_ASSERT(read_macho_text("/tmp/clang_id.o", &ct, &cn) == 0);
    const unsigned char *cid = NULL; size_t cidn = 0;
    T_ASSERT(nth_func(ct, cn, 0, &cid, &cidn) == 0);
    unsigned char *sumb = malloc(sumn);
    T_ASSERT(sumb != NULL);
    memcpy(sumb, sum, sumn);
    retarget_bl(sumb, sumn);
    fp = fopen("/tmp/fakecc_abi_fn.s", "w");
    T_ASSERT(fp != NULL);
    fputs(".text\n", fp);
    write_sym(fp, "sum", sumb, sumn);
    write_sym(fp, "id", cid, cidn);
    fclose(fp);
    T_ASSERT_EQ_INT(clang_exit("int sum(void);\nint main(void){ return sum(); }\n"), 11);
    free(sumb);
    free(ct);
    free(text);

    src = "package main;\n"
          "struct h2 { double a, b; };\n"
          "struct h2 id(struct h2 x){ return x; }\n"
          "int sum(void){ struct h2 x; unsigned long long *p;\n"
          " p=(unsigned long long *)&x; p[0]=1; p[1]=2;\n"
          " struct h2 y; y=id(x);\n"
          " unsigned long long *q=(unsigned long long *)&y;\n"
          " return q[0]==1 && q[1]==2; }\n"
          "int main(){ return 0; }\n";
    T_ASSERT(compile_image(src) == 0);
    T_ASSERT(read_macho_text("/tmp/fakecc_cg64_native_test", &text, &tn) == 0);
    T_ASSERT(nth_func(text, tn, 1, &id, &idn) == 0);
    T_ASSERT(nth_func(text, tn, 2, &sum, &sumn) == 0);
    fp = fopen("/tmp/fakecc_abi_fn.s", "w");
    T_ASSERT(fp != NULL);
    fputs(".text\n", fp);
    write_sym(fp, "id", id, idn);
    fclose(fp);
    T_ASSERT_EQ_INT(clang_exit(
        "struct h2 { double a, b; };\n"
        "struct h2 id(struct h2 x);\n"
        "int main(void){ struct h2 x; unsigned long long *p=(unsigned long long *)&x;\n"
        " p[0]=1; p[1]=2; struct h2 y=id(x);\n"
        " unsigned long long *q=(unsigned long long *)&y;\n"
        " return q[0]==1 && q[1]==2; }\n"), 1);
    fp = fopen("/tmp/clang_id.c", "w");
    fputs("struct h2 { double a, b; };\nstruct h2 id(struct h2 x){ return x; }\n", fp);
    fclose(fp);
    T_ASSERT(system("clang -arch arm64 -O0 -c /tmp/clang_id.c -o /tmp/clang_id.o") == 0);
    T_ASSERT(read_macho_text("/tmp/clang_id.o", &ct, &cn) == 0);
    T_ASSERT(nth_func(ct, cn, 0, &cid, &cidn) == 0);
    sumb = malloc(sumn);
    memcpy(sumb, sum, sumn);
    retarget_bl(sumb, sumn);
    fp = fopen("/tmp/fakecc_abi_fn.s", "w");
    fputs(".text\n", fp);
    write_sym(fp, "sum", sumb, sumn);
    write_sym(fp, "id", cid, cidn);
    fclose(fp);
    T_ASSERT_EQ_INT(clang_exit("int sum(void);\nint main(void){ return sum(); }\n"), 1);
    free(sumb);
    free(ct);
    free(text);
}

static void test_varargs(void) {
    /* Darwin puts every anonymous argument in an 8-byte stack slot.
     * va_list is the cursor, not a register-save area. */
    expect("va_one",
        "package main;\n"
        "int first(int n, ...){ va_list ap; va_start(ap, n);\n"
        " int x=va_arg(ap, int); va_end(ap); return x; }\n"
        "int main(){ return first(1, 5); }", 5);
    expect("va_neg",
        "package main;\n"
        "int first(int n, ...){ va_list ap; va_start(ap, n);\n"
        " int x=va_arg(ap, int); va_end(ap); return x; }\n"
        "int main(){ return first(1, -3); }", (unsigned char)-3);
    expect("va_sum10",
        "package main;\n"
        "int sum(int n, ...){ va_list ap; va_start(ap, n); int s=0;\n"
        " s=s+va_arg(ap, int); s=s+va_arg(ap, int); s=s+va_arg(ap, int);\n"
        " s=s+va_arg(ap, int); s=s+va_arg(ap, int); s=s+va_arg(ap, int);\n"
        " s=s+va_arg(ap, int); s=s+va_arg(ap, int); s=s+va_arg(ap, int);\n"
        " s=s+va_arg(ap, int); va_end(ap); return s; }\n"
        "int main(){ return sum(10, 1,2,3,4,5,6,7,8,9,10); }", 55);
    expect("va_after8",
        "package main;\n"
        "int many(int a,int b,int c,int d,int e,int f,int g,int h, ...){\n"
        " va_list ap; va_start(ap, h);\n"
        " int x=va_arg(ap, int); int y=va_arg(ap, int); va_end(ap);\n"
        " return a+x+y; }\n"
        "int main(){ return many(1,2,3,4,5,6,7,8, 9, 10); }", 20);
    expect("va_copy_same",
        "package main;\n"
        "int both(int n, ...){ va_list ap, bp; va_start(ap, n);\n"
        " va_copy(bp, ap);\n"
        " int a=va_arg(ap, int); int b=va_arg(bp, int);\n"
        " va_end(ap); va_end(bp); return a+b; }\n"
        "int main(){ return both(1, 10, 20); }", 20);
    expect("va_s16",
        "package main;\n"
        "struct s16 { long a; long b; };\n"
        "int f(int n, ...){ va_list ap; va_start(ap, n);\n"
        " struct s16 s; s=va_arg(ap, struct s16); int x=va_arg(ap, int);\n"
        " va_end(ap); return (int)(s.a+s.b+x); }\n"
        "int main(){ struct s16 s; s.a=10; s.b=20; return f(1, s, 3); }", 33);
    expect("va_s32",
        "package main;\n"
        "struct s32 { long a,b,c,d; };\n"
        "int f(int n, ...){ va_list ap; va_start(ap, n);\n"
        " struct s32 s; s=va_arg(ap, struct s32); va_end(ap);\n"
        " return (int)(s.a+s.b+s.c+s.d); }\n"
        "int main(){ struct s32 s; s.a=1; s.b=2; s.c=3; s.d=4; return f(0, s); }",
        10);
    expect("va_mixed",
        "package main;\n"
        "int f(int n, ...){ va_list ap; va_start(ap, n);\n"
        " int a=va_arg(ap, int);\n"
        " void *p=va_arg(ap, void *);\n"
        " unsigned long long bits=va_arg(ap, unsigned long long);\n"
        " int b=va_arg(ap, int); va_end(ap);\n"
        " if (p!=(void *)2) return 10;\n"
        " if ((bits>>32)!=0x3ff00000u) return 11;\n"
        " return a+b; }\n"
        "int main(){ return f(0, 1, (void *)2, 1.0, 3); }", 4);

    /* clang calls fakecc's sum3, then fakecc's check calls clang's sum3. */
    const char *src =
        "package main;\n"
        "int sum3(int n, ...){ va_list ap; va_start(ap, n);\n"
        " int a=va_arg(ap, int); int b=va_arg(ap, int); int c=va_arg(ap, int);\n"
        " va_end(ap); return a+b+c; }\n"
        "int check(void){ return sum3(3, 10, 20, 30); }\n"
        "int main(){ return 0; }\n";
    T_ASSERT(compile_image(src) == 0);
    unsigned char *text = NULL; size_t tn = 0;
    T_ASSERT(read_macho_text("/tmp/fakecc_cg64_native_test", &text, &tn) == 0);
    const unsigned char *sum3 = NULL, *check = NULL;
    size_t sum3n = 0, checkn = 0;
    T_ASSERT(nth_func(text, tn, 1, &sum3, &sum3n) == 0);
    T_ASSERT(nth_func(text, tn, 2, &check, &checkn) == 0);
    FILE *fp = fopen("/tmp/fakecc_abi_fn.s", "w");
    T_ASSERT(fp != NULL);
    fputs(".text\n", fp);
    write_sym(fp, "sum3", sum3, sum3n);
    fclose(fp);
    T_ASSERT_EQ_INT(clang_exit(
        "#include <stdarg.h>\n"
        "int sum3(int n, ...);\n"
        "int main(void){ return sum3(3, 10, 20, 30); }\n"), 60);

    fp = fopen("/tmp/clang_id.c", "w");
    T_ASSERT(fp != NULL);
    fputs("#include <stdarg.h>\n"
          "int sum3(int n, ...){ va_list ap; va_start(ap, n);\n"
          " int a=va_arg(ap, int); int b=va_arg(ap, int); int c=va_arg(ap, int);\n"
          " va_end(ap); return a+b+c; }\n", fp);
    fclose(fp);
    T_ASSERT(system("clang -arch arm64 -O0 -c /tmp/clang_id.c -o /tmp/clang_id.o") == 0);
    unsigned char *ct = NULL; size_t cn = 0;
    T_ASSERT(read_macho_text("/tmp/clang_id.o", &ct, &cn) == 0);
    const unsigned char *cid = NULL; size_t cidn = 0;
    T_ASSERT(nth_func(ct, cn, 0, &cid, &cidn) == 0);
    unsigned char *checkb = malloc(checkn);
    T_ASSERT(checkb != NULL);
    memcpy(checkb, check, checkn);
    retarget_bl(checkb, checkn);
    fp = fopen("/tmp/fakecc_abi_fn.s", "w");
    T_ASSERT(fp != NULL);
    fputs(".text\n", fp);
    write_sym(fp, "check", checkb, checkn);
    write_sym(fp, "sum3", cid, cidn);
    fclose(fp);
    T_ASSERT_EQ_INT(clang_exit("int check(void);\nint main(void){ return check(); }\n"), 60);
    free(checkb);
    free(ct);
    free(text);
}

static void test_struct_abi(void) {
    /* Sizes 1/2/3/7/8/9/16/17/32, nested, and a struct between ints.
     * Same source under clang must return the same code: both compilers
     * are internally consistent, so this locks the observable result.
     * Register assignment itself is checked by the cross-link test. */
    expect("s1",
        "package main;\n"
        "struct s1 { char a; };\n"
        "int f(struct s1 s){ return s.a; }\n"
        "int main(){ struct s1 s; s.a=41; return f(s); }", 41);
    expect("s2",
        "package main;\n"
        "struct s2 { char a[2]; };\n"
        "int f(struct s2 s){ return s.a[0]+s.a[1]; }\n"
        "int main(){ struct s2 s; s.a[0]=20; s.a[1]=22; return f(s); }", 42);
    expect("s3",
        "package main;\n"
        "struct s3 { char a[3]; };\n"
        "int f(struct s3 s){ return s.a[0]+s.a[1]+s.a[2]; }\n"
        "int main(){ struct s3 s; s.a[0]=1; s.a[1]=2; s.a[2]=3; return f(s); }", 6);
    expect("s7",
        "package main;\n"
        "struct s7 { char a[7]; };\n"
        "int f(struct s7 s){ return s.a[0]+s.a[6]; }\n"
        "int main(){ struct s7 s; s.a[0]=10; s.a[6]=7; return f(s); }", 17);
    expect("s8",
        "package main;\n"
        "struct s8 { long a; };\n"
        "long f(struct s8 s){ return s.a+1; }\n"
        "int main(){ struct s8 s; s.a=40; return (int)f(s); }", 41);
    expect("s9",
        "package main;\n"
        "struct s9 { char a[9]; };\n"
        "int f(struct s9 s){ return s.a[0]+s.a[8]; }\n"
        "int main(){ struct s9 s; s.a[0]=4; s.a[8]=5; return f(s); }", 9);
    expect("s16",
        "package main;\n"
        "struct s16 { long a, b; };\n"
        "struct s16 f(struct s16 s){ s.b += s.a; return s; }\n"
        "int main(){ struct s16 s; s.a=3; s.b=4; s=f(s); return (int)(s.a+s.b); }", 10);
    expect("s17",
        "package main;\n"
        "struct s17 { char a[17]; };\n"
        "struct s17 f(struct s17 s){ s.a[0]++; s.a[16]++; return s; }\n"
        "int main(){ struct s17 s; s.a[0]=1; s.a[16]=2; s=f(s);\n"
        " return s.a[0]+s.a[16]; }", 5);
    expect("s32",
        "package main;\n"
        "struct s32 { long a,b,c,d; };\n"
        "struct s32 f(struct s32 s){ s.d += s.a; return s; }\n"
        "int main(){ struct s32 s; s.a=1; s.b=2; s.c=3; s.d=4; s=f(s);\n"
        " return (int)(s.a+s.b+s.c+s.d); }", 11);
    expect("nested",
        "package main;\n"
        "struct inner { int a, b; };\n"
        "struct outer { struct inner i; int c; };\n"
        "int f(struct outer o){ return o.i.a + o.i.b + o.c; }\n"
        "int main(){ struct outer o; o.i.a=1; o.i.b=2; o.c=3; return f(o); }", 6);
    expect("many_s16",
        "package main;\n"
        "struct s16 { long a, b; };\n"
        "int many(int a, struct s16 s, int b){ return a+(int)s.a+(int)s.b+b; }\n"
        "int main(){ struct s16 s; s.a=10; s.b=20; return many(1,s,3); }", 34);
    expect("after7",
        "package main;\n"
        "struct s8 { long a; };\n"
        "int f(int a0,int a1,int a2,int a3,int a4,int a5,int a6,\n"
        "      struct s8 s, int a8){\n"
        " return a0+a1+a2+a3+a4+a5+a6+(int)s.a+a8; }\n"
        "int main(){ struct s8 s; s.a=10;\n"
        " return f(1,1,1,1,1,1,1,s,1); }", 18);
    expect("hfa2",
        "package main;\n"
        "struct h2 { double a, b; };\n"
        "struct h2 id(struct h2 x){ return x; }\n"
        "int main(){ struct h2 x; unsigned long long *p;\n"
        " p=(unsigned long long *)&x; p[0]=1; p[1]=2;\n"
        " struct h2 y; y=id(x);\n"
        " unsigned long long *q=(unsigned long long *)&y;\n"
        " return q[0]==1 && q[1]==2; }", 1);
    expect("hfa4",
        "package main;\n"
        "struct h4 { float a,b,c,d; };\n"
        "struct h4 id(struct h4 x){ return x; }\n"
        "int main(){ struct h4 x; unsigned *p=(unsigned *)&x;\n"
        " p[0]=1; p[1]=2; p[2]=3; p[3]=4;\n"
        " struct h4 y; y=id(x); unsigned *q=(unsigned *)&y;\n"
        " return (int)(q[0]+q[1]+q[2]+q[3]); }", 10);
    expect("mixed",
        "package main;\n"
        "struct m { int a; double b; };\n"
        "int f(struct m s){ unsigned long long u; u=*(unsigned long long *)&s.b;\n"
        " return s.a + (int)u; }\n"
        "int main(){ struct m s; unsigned long long *p;\n"
        " s.a=7; p=(unsigned long long *)&s.b; *p=4; return f(s); }", 11);
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
    test_struct_abi();
    test_abi_cross();
    test_varargs();
    return t_finalize();
}

#else

int main(void) { return t_finalize(); }  /* non-Apple-Silicon: no-op */

#endif
