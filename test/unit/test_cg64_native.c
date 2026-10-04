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
#include "fakecc/compiler.h"
#include "fakecc/macho.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <fcntl.h>
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
    if (WIFEXITED(st)) return WEXITSTATUS(st);
    if (WIFSIGNALED(st)) return 128 + WTERMSIG(st);
    return -1008;
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

static void test_float(void) {
    expect("fadd",
        "package main;\n"
        "int main(){ double a=1.5; double b=2.5; return (int)(a+b); }", 4);
    expect("fsub_mul_div",
        "package main;\n"
        "int main(){ double a=7.5; double b=2.5;\n"
        " return (int)((a-b) + (a/b) + (b*b)); }", 14);
    expect("f32_add",
        "package main;\n"
        "int main(){ float a=1.5f; float b=2.25f; return (int)(a+b); }", 3);
    expect("fcmp_ord",
        "package main;\n"
        "int main(){ double a=1.5; double b=2.5;\n"
        " return (a<b) + (a<=a) + (b>a) + (a>=a) + (a==a) + (a!=b); }", 6);
    expect("fcmp_nan",
        "package main;\n"
        "int main(){ double n=0.0/0.0;\n"
        " return (n!=0.0) && !(n==0.0) && !(n<1.0) && !(n<=1.0)\n"
        "     && !(n>1.0) && !(n>=1.0); }", 1);
    expect("fcvt_int",
        "package main;\n"
        "int main(){ double d=(double)3 + 0.9; float f=(float)d;\n"
        " unsigned u=4000000000u; double ud=(double)u;\n"
        " return ((int)d==3) && ((int)f==3) && (ud>3999999999.0); }", 1);
    expect("fparam",
        "package main;\n"
        "double add(double a, double b){ return a+b; }\n"
        "int main(){ return (int)add(1.25, 2.75); }", 4);
    expect("fneg",
        "package main;\n"
        "int main(){ double a=2.5; return (int)(-a + 5.0); }", 2);
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
    expect("va_dbl",
        "package main;\n"
        "int mixed(int n, ...){ va_list ap; va_start(ap, n);\n"
        " int a=va_arg(ap, int); double b=va_arg(ap, double);\n"
        " va_end(ap); return a+(int)b; }\n"
        "int main(){ double x=3.5; return mixed(2, 5, x); }", 8);
    expect("va_dbl_const",
        "package main;\n"
        "int mixed(int n, ...){ va_list ap; va_start(ap, n);\n"
        " int a=va_arg(ap, int); double b=va_arg(ap, double);\n"
        " va_end(ap); return a+(int)b; }\n"
        "int main(){ return mixed(2, 5, 3.5); }", 8);
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

static void test_vec16(void) {
    expect("v16d",
        "package main;\n"
        "typedef double V __attribute__((vector_size(16)));\n"
        "V id(V v) { return v; }\n"
        "int main(void) {\n"
        "  V a = { 1.0, 2.0 };\n"
        "  V b = id(a);\n"
        "  if (b[0] != 1.0) return 1;\n"
        "  if (b[1] != 2.0) return 2;\n"
        "  return 0; }", 0);
    expect("v16i",
        "package main;\n"
        "typedef int V __attribute__((vector_size(16)));\n"
        "V id(V v) { return v; }\n"
        "int main(void) {\n"
        "  V a = { 1, 2, 3, 4 };\n"
        "  V b = id(a);\n"
        "  return b[0] + b[1] + b[2] + b[3]; }", 10);
    expect("v16mix",
        "package main;\n"
        "typedef int V __attribute__((vector_size(16)));\n"
        "int f(int n, V v) { return n + v[0] + v[3]; }\n"
        "int main(void) {\n"
        "  V a = { 1, 2, 3, 4 };\n"
        "  return f(10, a); }", 15);
    expect("vadd",
        "package main;\n"
        "typedef int V __attribute__((vector_size(16)));\n"
        "__attribute__((noinline)) int after(int n, V r) {\n"
        "  if (r[0] != 6) return 10;\n"
        "  if (r[1] != 8) return 11;\n"
        "  if (r[2] != 10) return 12;\n"
        "  if (r[3] != 12) return 13;\n"
        "  return n; }\n"
        "int main(void) {\n"
        "  int n = 7;\n"
        "  V a = { 1, 2, 3, 4 };\n"
        "  V b = { 5, 6, 7, 8 };\n"
        "  V r = a + b;\n"
        "  n = n + 1;\n"
        "  if (after(n, r) != 8) return 1;\n"
        "  return 0; }", 0);
    expect("vsubmul",
        "package main;\n"
        "typedef int V __attribute__((vector_size(16)));\n"
        "int main(void) {\n"
        "  V a = { 9, 8, 7, 6 };\n"
        "  V b = { 1, 2, 3, 4 };\n"
        "  V s = a - b;\n"
        "  V m = b * b;\n"
        "  return s[0] + s[3] + m[1] + m[2]; }", 23);
    expect("vbit",
        "package main;\n"
        "typedef int V __attribute__((vector_size(16)));\n"
        "int main(void) {\n"
        "  V a = { 0x0f, 0xf0, 0xff, 0x11 };\n"
        "  V b = { 0x33, 0x0f, 0x0f, 0x22 };\n"
        "  V u = a & b;\n"
        "  V o = a | b;\n"
        "  V x = a ^ b;\n"
        "  if (u[0] != 3) return 1;\n"
        "  if (o[1] != 255) return 2;\n"
        "  if (x[3] != 0x33) return 3;\n"
        "  return 0; }", 0);
    expect("vfadd",
        "package main;\n"
        "typedef double V __attribute__((vector_size(16)));\n"
        "int main(void) {\n"
        "  V a = { 1.5, 2.5 };\n"
        "  V b = { 2.5, 1.5 };\n"
        "  V r = a + b;\n"
        "  return (int)r[0] + (int)r[1]; }", 8);
    expect("v16stk",
        "package main;\n"
        "typedef double V __attribute__((vector_size(16)));\n"
        "__attribute__((noinline)) V last(V a, V b, V c, V d, V e, V f, V g, V h, V i) {\n"
        "  (void)a; (void)b; (void)c; (void)d;\n"
        "  (void)e; (void)f; (void)g; (void)h;\n"
        "  return i; }\n"
        "int main(void) {\n"
        "  V z = { 0.0, 0.0 };\n"
        "  V x = { 3.0, 4.0 };\n"
        "  V r = last(z, z, z, z, z, z, z, z, x);\n"
        "  if (r[0] != 3.0) return 1;\n"
        "  if (r[1] != 4.0) return 2;\n"
        "  return 0; }", 0);
    expect("vmul8",
        "package main;\n"
        "typedef unsigned char V __attribute__((vector_size(16)));\n"
        "int main(void) {\n"
        "  V a = { 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18 };\n"
        "  V b = { 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2 };\n"
        "  V r = a * b;\n"
        "  return r[0] + r[15]; }", 42);
    expect("vmul64",
        "package main;\n"
        "typedef long long V __attribute__((vector_size(16)));\n"
        "int main(void) {\n"
        "  V a = { 20, 7 };\n"
        "  V b = { 3, 4 };\n"
        "  V r = a * b;\n"
        "  return (int)(r[0] + r[1]); }", 88);
    expect("v32id",
        "package main;\n"
        "typedef int V __attribute__((vector_size(32)));\n"
        "__attribute__((noinline)) V id(V v) { return v; }\n"
        "int main(void) {\n"
        "  V a = { 1, 2, 3, 4, 5, 6, 7, 8 };\n"
        "  V b = id(a);\n"
        "  if (b[0] != 1) return 1;\n"
        "  if (b[3] != 4) return 2;\n"
        "  if (b[7] != 8) return 3;\n"
        "  return 0; }", 0);
    expect("v64id",
        "package main;\n"
        "typedef int V __attribute__((vector_size(64)));\n"
        "__attribute__((noinline)) V id(V v) { return v; }\n"
        "int main(void) {\n"
        "  V a = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16 };\n"
        "  V b = id(a);\n"
        "  if (b[0] != 1) return 1;\n"
        "  if (b[7] != 8) return 2;\n"
        "  if (b[15] != 16) return 3;\n"
        "  return 0; }", 0);
    expect("ldbl",
        "package main;\n"
        "long double add(long double x, long double y) { return x + y; }\n"
        "int main(void) {\n"
        "  if (sizeof(long double) != 8) return 1;\n"
        "  long double a = 1.5L;\n"
        "  if ((int)(a + 2.5L) != 4) return 2;\n"
        "  if ((int)add(1.25L, 2.75L) != 4) return 3;\n"
        "  return 0; }", 0);
}

static void test_macho_obj(void) {
    const char *path = "/tmp/fakecc_arm64_obj.o";
    int rc = fakecc_compile_string_to_obj(
        "package main;\n"
        "static int hidden(int x) { return x + 1; }\n"
        "int add(int a, int b) { return a + b + hidden(a); }\n",
        path, NULL);
    T_ASSERT_EQ_INT(rc, 0);
    FILE *f = fopen(path, "rb");
    T_ASSERT(f != NULL);
    unsigned char hdr[32];
    T_ASSERT_EQ_INT((int)fread(hdr, 1, 32, f), 32);
    uint32_t magic = 0, filetype = 0, flags = 0;
    memcpy(&magic, hdr, 4);
    memcpy(&filetype, hdr + 12, 4);
    memcpy(&flags, hdr + 24, 4);
    T_ASSERT_EQ_INT((int)magic, (int)0xFEEDFACF);
    T_ASSERT_EQ_INT((int)filetype, 1);
    T_ASSERT_EQ_INT((int)(flags & 0x2000), 0x2000);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    rewind(f);
    char *buf = malloc((size_t)n);
    T_ASSERT(buf != NULL);
    T_ASSERT_EQ_INT((int)fread(buf, 1, (size_t)n, f), (int)n);
    fclose(f);
    T_ASSERT(memmem(buf, (size_t)n, "_add", 4) != NULL);
    T_ASSERT(memmem(buf, (size_t)n, "_hidden", 7) != NULL);
    free(buf);

    const char *gpath = "/tmp/fakecc_arm64_gobj.o";
    rc = fakecc_compile_string_to_obj(
        "package main;\n"
        "int g = 7;\n"
        "int get(void) { return g; }\n",
        gpath, NULL);
    T_ASSERT_EQ_INT(rc, 0);
    f = fopen(gpath, "rb");
    T_ASSERT(f != NULL);
    unsigned char sec[80];
    T_ASSERT_EQ_INT((int)fseek(f, 32 + 72, SEEK_SET), 0);
    T_ASSERT_EQ_INT((int)fread(sec, 1, 80, f), 80);
    fclose(f);
    uint32_t nreloc = 0;
    memcpy(&nreloc, sec + 60, 4);
    T_ASSERT_EQ_INT((int)nreloc, 2);

    const char *ppath = "/tmp/fakecc_arm64_pobj.o";
    rc = fakecc_compile_string_to_obj(
        "package main;\n"
        "int g = 1;\n"
        "int *p = &g;\n",
        ppath, NULL);
    T_ASSERT_EQ_INT(rc, 0);
    f = fopen(ppath, "rb");
    T_ASSERT(f != NULL);
    /* header, segment, __text, then __data. nreloc is 60 bytes in. */
    T_ASSERT_EQ_INT((int)fseek(f, 32 + 72 + 80 + 60, SEEK_SET), 0);
    uint32_t dnreloc = 0;
    T_ASSERT_EQ_INT((int)fread(&dnreloc, 4, 1, f), 1);
    fclose(f);
    T_ASSERT_EQ_INT((int)dnreloc, 1);

    const char *xpath = "/tmp/fakecc_arm64_xobj.o";
    rc = fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int other(int x);\n"
        "int call(int x) { return other(x); }\n",
        xpath, NULL);
    T_ASSERT_EQ_INT(rc, 0);
    f = fopen(xpath, "rb");
    T_ASSERT(f != NULL);
    T_ASSERT_EQ_INT((int)fseek(f, 32 + 72 + 60, SEEK_SET), 0);
    uint32_t xnreloc = 0;
    T_ASSERT_EQ_INT((int)fread(&xnreloc, 4, 1, f), 1);
    fclose(f);
    T_ASSERT_EQ_INT((int)xnreloc, 2);

    EmitModule back;
    T_ASSERT_EQ_INT(emit_obj_read(xpath, &back), 0);
    T_ASSERT(back.text.len > 0);
    T_ASSERT_EQ_INT((int)back.num_relocs, 2);
    T_ASSERT(emit_module_find_symbol(&back, "call") >= 0);
    int other = emit_module_find_symbol(&back, "other");
    T_ASSERT(other >= 0);
    T_ASSERT_EQ_INT(back.syms[other].shndx, 0);
    int saw5 = 0, saw6 = 0;
    for (size_t i = 0; i < back.num_relocs; i++) {
        if (back.relocs[i].type == 5) saw5 = 1;
        if (back.relocs[i].type == 6) saw6 = 1;
    }
    T_ASSERT(saw5 && saw6);
    emit_module_free(&back);

    T_ASSERT_EQ_INT(emit_obj_read(ppath, &back), 0);
    T_ASSERT(back.data.len > 0);
    T_ASSERT_EQ_INT((int)back.num_data_relocs, 1);
    T_ASSERT(emit_module_find_symbol(&back, "g") >= 0);
    T_ASSERT(emit_module_find_symbol(&back, "p") >= 0);
    emit_module_free(&back);
}

static int run_bin(const char *path) {
    pid_t pid = fork();
    if (pid < 0) return -1;
    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(127);
    }
    int status = 0;
    waitpid(pid, &status, 0);
    if (!WIFEXITED(status)) return -1;
    return WEXITSTATUS(status);
}

/* Linker warnings go to stderr.  Restore it before the assertion runs. */
static int link_capturing(EmitModule **mods, size_t n, const char *out,
                          const char *errpath) {
    int saved = dup(STDERR_FILENO);
    int fd = open(errpath, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (saved < 0 || fd < 0) return -1;
    dup2(fd, STDERR_FILENO);
    close(fd);
    int rc = macho_link_objects(mods, n, out);
    fflush(stderr);
    dup2(saved, STDERR_FILENO);
    close(saved);
    return rc;
}

static int err_has(const char *path, const char *needle) {
    FILE *f = fopen(path, "r");
    if (!f) return 0;
    char buf[1024];
    size_t n = fread(buf, 1, sizeof buf - 1, f);
    fclose(f);
    buf[n] = 0;
    return strstr(buf, needle) != NULL;
}

static void test_common(void) {
    const char *a = "/tmp/fakecc_arm64_common_a.o";
    const char *b = "/tmp/fakecc_arm64_common_b.o";
    const char *outp = "/tmp/fakecc_arm64_common";
    const char *err = "/tmp/fakecc_arm64_common_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int big[4];\n"
        "int main(void) { return big[0]; }\n",
        a, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int big[1] = { 7 };\n",
        b, NULL), 0);
    EmitModule ma, mb;
    T_ASSERT_EQ_INT(emit_obj_read(a, &ma), 0);
    T_ASSERT_EQ_INT(emit_obj_read(b, &mb), 0);
    EmitModule *mods[2] = { &ma, &mb };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT(err_has(err, "tentative definition of 'big'"));
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&ma);
    emit_module_free(&mb);

    const char *c = "/tmp/fakecc_arm64_common_c.o";
    const char *d = "/tmp/fakecc_arm64_common_d.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int wide[1];\n"
        "int main(void) { return wide[0] + wide[1]; }\n",
        c, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int wide[2] = { 5, 6 };\n",
        d, NULL), 0);
    EmitModule mc, md;
    T_ASSERT_EQ_INT(emit_obj_read(c, &mc), 0);
    T_ASSERT_EQ_INT(emit_obj_read(d, &md), 0);
    EmitModule *mods2[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods2, 2, outp, err), 0);
    T_ASSERT(!err_has(err, "tentative definition"));
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 11);
    emit_module_free(&mc);
    emit_module_free(&md);
}

static void test_weak(void) {
    const char *wk = "/tmp/fakecc_arm64_weak.o";
    const char *st = "/tmp/fakecc_arm64_strong.o";
    const char *st2 = "/tmp/fakecc_arm64_strong2.o";
    const char *call = "/tmp/fakecc_arm64_weak_main.o";
    const char *outp = "/tmp/fakecc_arm64_weak_out";
    const char *err = "/tmp/fakecc_arm64_weak_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int pick(void) __attribute__((weak)) { return 1; }\n",
        wk, NULL), 0);
    EmitModule mwk;
    T_ASSERT_EQ_INT(emit_obj_read(wk, &mwk), 0);
    int ps = emit_module_find_symbol(&mwk, "pick");
    T_ASSERT(ps >= 0);
    T_ASSERT_EQ_INT((int)mwk.syms[ps].binding, 2);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int pick(void);\n"
        "int main(void) { return pick(); }\n",
        call, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int pick(void) { return 2; }\n",
        st, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int pick(void) { return 3; }\n",
        st2, NULL), 0);
    EmitModule mcall, mst, mst2;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mcall), 0);
    T_ASSERT_EQ_INT(emit_obj_read(st, &mst), 0);
    T_ASSERT_EQ_INT(emit_obj_read(st2, &mst2), 0);
    EmitModule *only[2] = { &mcall, &mwk };
    T_ASSERT_EQ_INT(link_capturing(only, 2, outp, err), 0);
    T_ASSERT(!err_has(err, "duplicate symbol"));
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 1);
    EmitModule *win[3] = { &mcall, &mwk, &mst };
    T_ASSERT_EQ_INT(link_capturing(win, 3, outp, err), 0);
    T_ASSERT(!err_has(err, "duplicate symbol"));
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 2);
    EmitModule *rev[3] = { &mcall, &mst, &mwk };
    T_ASSERT_EQ_INT(link_capturing(rev, 3, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 2);
    EmitModule *dup[3] = { &mcall, &mst, &mst2 };
    T_ASSERT(link_capturing(dup, 3, outp, err) != 0);
    T_ASSERT(err_has(err, "duplicate symbol"));
    emit_module_free(&mwk);
    emit_module_free(&mcall);
    emit_module_free(&mst);
    emit_module_free(&mst2);
}

static void test_wref(void) {
    const char *call = "/tmp/fakecc_arm64_wref_main.o";
    const char *def = "/tmp/fakecc_arm64_wref_def.o";
    const char *gcall = "/tmp/fakecc_arm64_wref_g.o";
    const char *gdef = "/tmp/fakecc_arm64_wref_gd.o";
    const char *outp = "/tmp/fakecc_arm64_wref_out";
    const char *err = "/tmp/fakecc_arm64_wref_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int missing(void) __attribute__((weak));\n"
        "int main(void) {\n"
        "  if (!missing) return 7;\n"
        "  return missing();\n"
        "}\n",
        call, NULL), 0);
    EmitModule mc;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    int ms = emit_module_find_symbol(&mc, "missing");
    T_ASSERT(ms >= 0);
    T_ASSERT_EQ_INT((int)mc.syms[ms].binding, 2);
    T_ASSERT_EQ_INT((int)mc.syms[ms].shndx, 0);
    EmitModule *alone[1] = { &mc };
    T_ASSERT_EQ_INT(link_capturing(alone, 1, outp, err), 0);
    T_ASSERT(!err_has(err, "undefined symbol"));
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int missing(void) { return 3; }\n",
        def, NULL), 0);
    EmitModule md;
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    EmitModule *both[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(both, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 3);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int val __attribute__((weak));\n"
        "int main(void) {\n"
        "  if (!&val) return 7;\n"
        "  return val;\n"
        "}\n",
        gcall, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int val = 4;\n",
        gdef, NULL), 0);
    EmitModule mg, mgd;
    T_ASSERT_EQ_INT(emit_obj_read(gcall, &mg), 0);
    T_ASSERT_EQ_INT(emit_obj_read(gdef, &mgd), 0);
    EmitModule *gonly[1] = { &mg };
    T_ASSERT_EQ_INT(link_capturing(gonly, 1, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    EmitModule *gboth[2] = { &mg, &mgd };
    T_ASSERT_EQ_INT(link_capturing(gboth, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 4);
    emit_module_free(&mc);
    emit_module_free(&md);
    emit_module_free(&mg);
    emit_module_free(&mgd);
}

static void test_alias(void) {
    const char *def = "/tmp/fakecc_arm64_alias_def.o";
    const char *call = "/tmp/fakecc_arm64_alias_main.o";
    const char *strong = "/tmp/fakecc_arm64_alias_strong.o";
    const char *gdef = "/tmp/fakecc_arm64_alias_g.o";
    const char *gcall = "/tmp/fakecc_arm64_alias_gm.o";
    const char *outp = "/tmp/fakecc_arm64_alias_out";
    const char *err = "/tmp/fakecc_arm64_alias_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int real(void) { return 4; }\n"
        "int alias(void) __attribute__((alias(\"real\")));\n",
        def, NULL), 0);
    EmitModule md;
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    int rs = emit_module_find_symbol(&md, "real");
    int as = emit_module_find_symbol(&md, "alias");
    T_ASSERT(rs >= 0 && as >= 0);
    T_ASSERT_EQ_INT((int)md.syms[rs].value, (int)md.syms[as].value);
    T_ASSERT_EQ_INT((int)md.syms[as].binding, 1);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int alias(void);\n"
        "int main(void) { return alias(); }\n",
        call, NULL), 0);
    EmitModule mc;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 4);
    const char *wdef = "/tmp/fakecc_arm64_alias_w.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int real(void) { return 4; }\n"
        "int alias(void) __attribute__((weak, alias(\"real\")));\n",
        wdef, NULL), 0);
    EmitModule mw;
    T_ASSERT_EQ_INT(emit_obj_read(wdef, &mw), 0);
    int ws = emit_module_find_symbol(&mw, "alias");
    T_ASSERT(ws >= 0);
    T_ASSERT_EQ_INT((int)mw.syms[ws].binding, 2);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int alias(void) { return 9; }\n",
        strong, NULL), 0);
    EmitModule ms;
    T_ASSERT_EQ_INT(emit_obj_read(strong, &ms), 0);
    EmitModule *over[3] = { &mc, &mw, &ms };
    T_ASSERT_EQ_INT(link_capturing(over, 3, outp, err), 0);
    T_ASSERT(!err_has(err, "duplicate symbol"));
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 9);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int real = 6;\n"
        "int mirror __attribute__((alias(\"real\")));\n",
        gdef, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int mirror;\n"
        "int main(void) { return mirror; }\n",
        gcall, NULL), 0);
    EmitModule mg, mgc;
    T_ASSERT_EQ_INT(emit_obj_read(gdef, &mg), 0);
    T_ASSERT_EQ_INT(emit_obj_read(gcall, &mgc), 0);
    int gr = emit_module_find_symbol(&mg, "real");
    int gm = emit_module_find_symbol(&mg, "mirror");
    T_ASSERT(gr >= 0 && gm >= 0);
    T_ASSERT_EQ_INT((int)mg.syms[gr].value, (int)mg.syms[gm].value);
    EmitModule *gmods[2] = { &mgc, &mg };
    T_ASSERT_EQ_INT(link_capturing(gmods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 6);
    emit_module_free(&md);
    emit_module_free(&mc);
    emit_module_free(&mw);
    emit_module_free(&ms);
    emit_module_free(&mg);
    emit_module_free(&mgc);
}

static void test_hidden(void) {
    const char *def = "/tmp/fakecc_arm64_hidden.o";
    const char *call = "/tmp/fakecc_arm64_hidden_main.o";
    const char *outp = "/tmp/fakecc_arm64_hidden_out";
    const char *err = "/tmp/fakecc_arm64_hidden_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int hidden_add(int x) __attribute__((visibility(\"hidden\"))) {\n"
        "  return x + 1;\n"
        "}\n"
        "int shown = 5;\n"
        "int priv __attribute__((visibility(\"hidden\"))) = 6;\n",
        def, NULL), 0);
    EmitModule md;
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    int hf = emit_module_find_symbol(&md, "hidden_add");
    int hs = emit_module_find_symbol(&md, "shown");
    int hp = emit_module_find_symbol(&md, "priv");
    T_ASSERT(hf >= 0 && hs >= 0 && hp >= 0);
    T_ASSERT_EQ_INT((int)md.syms[hf].binding, 3);
    T_ASSERT_EQ_INT((int)md.syms[hs].binding, 1);
    T_ASSERT_EQ_INT((int)md.syms[hp].binding, 3);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int hidden_add(int x);\n"
        "extern int priv;\n"
        "int main(void) { return hidden_add(priv); }\n",
        call, NULL), 0);
    EmitModule mc;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&md);
    emit_module_free(&mc);
}

static int reloc_addend_is(const EmitModule *m, int addend) {
    for (size_t i = 0; i < m->num_relocs; i++)
        if (m->relocs[i].addend == addend) return 1;
    return 0;
}

static void test_addend(void) {
    const char *def = "/tmp/fakecc_arm64_addend_def.o";
    const char *call = "/tmp/fakecc_arm64_addend_main.o";
    const char *outp = "/tmp/fakecc_arm64_addend_out";
    const char *err = "/tmp/fakecc_arm64_addend_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "struct S { int a; int b; };\n"
        "struct S g;\n"
        "struct B { char pad[5000]; int x; };\n"
        "struct B wide;\n"
        "void set_fields(void) { g.b = 9; wide.x = 7; }\n",
        def, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "struct S { int a; int b; };\n"
        "extern struct S g;\n"
        "struct B { char pad[5000]; int x; };\n"
        "extern struct B wide;\n"
        "void set_fields(void);\n"
        "int main(void) {\n"
        "  set_fields();\n"
        "  if (g.b != 9) return 1;\n"
        "  return wide.x;\n"
        "}\n",
        call, NULL), 0);
    EmitModule mc;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    T_ASSERT(reloc_addend_is(&mc, 4));
    T_ASSERT(reloc_addend_is(&mc, 5000));
    EmitModule md;
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&mc);
    emit_module_free(&md);
}

static void test_used(void) {
    const char *obj = "/tmp/fakecc_arm64_used.o";
    const char *outp = "/tmp/fakecc_arm64_used_out";
    const char *err = "/tmp/fakecc_arm64_used_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "static int quiet(void) __attribute__((used)) { return 3; }\n"
        "int keeper(void) __attribute__((used)) { return 4; }\n"
        "int plain(void) { return 1; }\n"
        "int main(void) { return keeper(); }\n",
        obj, NULL), 0);
    EmitModule m;
    T_ASSERT_EQ_INT(emit_obj_read(obj, &m), 0);
    int q = emit_module_find_symbol(&m, "quiet");
    int k = emit_module_find_symbol(&m, "keeper");
    int p = emit_module_find_symbol(&m, "plain");
    T_ASSERT(q >= 0 && k >= 0 && p >= 0);
    T_ASSERT_EQ_INT((int)(m.syms[q].macho_desc & 0x0020), 0x0020);
    T_ASSERT_EQ_INT((int)(m.syms[k].macho_desc & 0x0020), 0x0020);
    T_ASSERT_EQ_INT((int)(m.syms[p].macho_desc & 0x0020), 0);
    T_ASSERT_EQ_INT((int)m.syms[q].binding, 0);
    EmitModule *mods[1] = { &m };
    T_ASSERT_EQ_INT(link_capturing(mods, 1, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 4);
    emit_module_free(&m);
}

static void test_subtractor(void) {
    const char *obj = "/tmp/fakecc_arm64_sub.o";
    const char *outp = "/tmp/fakecc_arm64_sub_out";
    const char *err = "/tmp/fakecc_arm64_sub_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int aaa = 1;\n"
        "int bbb = 2;\n"
        "long gap = (long)&bbb - (long)&aaa;\n"
        "int main(void) { return (int)gap; }\n",
        obj, NULL), 0);
    EmitModule m;
    T_ASSERT_EQ_INT(emit_obj_read(obj, &m), 0);
    int saw = 0;
    for (size_t i = 0; i < m.num_data_relocs; i++)
        if (m.data_relocs[i].type == 1) saw = 1;
    T_ASSERT(saw);
    EmitModule *mods[1] = { &m };
    T_ASSERT_EQ_INT(link_capturing(mods, 1, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 4);
    emit_module_free(&m);
}

static void test_got(void) {
    const char *call = "/tmp/fakecc_arm64_got_main.o";
    const char *def = "/tmp/fakecc_arm64_got_def.o";
    const char *outp = "/tmp/fakecc_arm64_got_out";
    const char *err = "/tmp/fakecc_arm64_got_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int val;\n"
        "int main(void) { return val; }\n",
        call, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int val = 6;\n",
        def, NULL), 0);
    EmitModule mc, md;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    int saw = 0;
    for (size_t i = 0; i < mc.num_relocs; i++)
        if (mc.relocs[i].type == 5) saw = 1;
    T_ASSERT(saw);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 6);
    emit_module_free(&mc);
    emit_module_free(&md);
}

static void test_fn_got(void) {
    const char *call = "/tmp/fakecc_arm64_fngot_main.o";
    const char *def = "/tmp/fakecc_arm64_fngot_def.o";
    const char *outp = "/tmp/fakecc_arm64_fngot_out";
    const char *err = "/tmp/fakecc_arm64_fngot_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int add1(int x);\n"
        "int main(void) {\n"
        "  int (*p)(int) = add1;\n"
        "  return p(5);\n"
        "}\n",
        call, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add1(int x) { return x + 1; }\n",
        def, NULL), 0);
    EmitModule mc, md;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    int saw = 0;
    for (size_t i = 0; i < mc.num_relocs; i++)
        if (mc.relocs[i].type == 5) saw = 1;
    T_ASSERT(saw);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 6);
    emit_module_free(&mc);
    emit_module_free(&md);
}

static void test_call_stub(void) {
    const char *call = "/tmp/fakecc_arm64_stub_main.o";
    const char *def = "/tmp/fakecc_arm64_stub_def.o";
    const char *outp = "/tmp/fakecc_arm64_stub_out";
    const char *err = "/tmp/fakecc_arm64_stub_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int add1(int x);\n"
        "int main(void) { return add1(5); }\n",
        call, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add1(int x) { return x + 1; }\n",
        def, NULL), 0);
    EmitModule mc, md;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    int saw_got = 0, saw_bl = 0;
    for (size_t i = 0; i < mc.num_relocs; i++) {
        if (mc.relocs[i].type == 5) saw_got = 1;
        if (mc.relocs[i].type == 2) saw_bl = 1;
    }
    T_ASSERT(saw_got);
    T_ASSERT(!saw_bl);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 6);
    emit_module_free(&mc);
    emit_module_free(&md);
}

static void test_tls(void) {
    expect("tls_inc",
        "package main;\n"
        "__thread int t = 3;\n"
        "int bump(void) { t += 1; return t; }\n"
        "int main(void) { return bump(); }\n", 4);
    const char *call = "/tmp/fakecc_arm64_tls_main.o";
    const char *def = "/tmp/fakecc_arm64_tls_def.o";
    const char *outp = "/tmp/fakecc_arm64_tls_out";
    const char *err = "/tmp/fakecc_arm64_tls_err.txt";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern __thread int t;\n"
        "int main(void) { t += 2; return t; }\n",
        call, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "__thread int t = 5;\n",
        def, NULL), 0);
    EmitModule mc, md;
    T_ASSERT_EQ_INT(emit_obj_read(call, &mc), 0);
    T_ASSERT_EQ_INT(emit_obj_read(def, &md), 0);
    int saw = 0;
    for (size_t i = 0; i < mc.num_relocs; i++)
        if (mc.relocs[i].type == 8) saw = 1;
    T_ASSERT(saw);
    EmitModule *mods[2] = { &mc, &md };
    T_ASSERT_EQ_INT(link_capturing(mods, 2, outp, err), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&mc);
    emit_module_free(&md);
}

static void test_malloc(void) {
    expect("malloc_roundtrip",
        "package main;\n"
        "void *malloc(unsigned long n);\n"
        "void free(void *p);\n"
        "int main(void) {\n"
        "  char *p = malloc(4);\n"
        "  if (!p) return 1;\n"
        "  p[0] = 9;\n"
        "  int v = p[0];\n"
        "  free(p);\n"
        "  free(0);\n"
        "  return v;\n"
        "}\n", 9);
    expect("malloc_user",
        "package main;\n"
        "void *malloc(unsigned long n) { return n ? 0 : 0; }\n"
        "int main(void) { return malloc(4) ? 1 : 7; }\n", 7);
    expect("calloc_zero",
        "package main;\n"
        "void *calloc(unsigned long n, unsigned long sz);\n"
        "int main(void) {\n"
        "  char *p = calloc(4, 4);\n"
        "  if (!p) return 1;\n"
        "  int i;\n"
        "  for (i = 0; i < 16; i++) if (p[i]) return 2;\n"
        "  p[15] = 6;\n"
        "  return p[15];\n"
        "}\n", 6);
    expect("calloc_overflow",
        "package main;\n"
        "void *calloc(unsigned long n, unsigned long sz);\n"
        "int main(void) {\n"
        "  unsigned long n = 1ul << 32;\n"
        "  return calloc(n, n) ? 1 : 7;\n"
        "}\n", 7);
    expect("realloc_grow",
        "package main;\n"
        "void *malloc(unsigned long n);\n"
        "void *realloc(void *p, unsigned long n);\n"
        "int main(void) {\n"
        "  char *p = malloc(4);\n"
        "  if (!p) return 1;\n"
        "  p[0] = 1; p[1] = 2; p[2] = 3; p[3] = 4;\n"
        "  char *q = realloc(p, 8);\n"
        "  if (!q) return 2;\n"
        "  if (q[0] != 1 || q[3] != 4) return 3;\n"
        "  q[7] = 5;\n"
        "  return q[0] + q[7];\n"
        "}\n", 6);
    expect("realloc_null",
        "package main;\n"
        "void *realloc(void *p, unsigned long n);\n"
        "int main(void) {\n"
        "  char *p = realloc(0, 4);\n"
        "  if (!p) return 1;\n"
        "  p[0] = 8;\n"
        "  int v = p[0];\n"
        "  char *z = realloc(p, 0);\n"
        "  return z ? 2 : v;\n"
        "}\n", 8);
}

static void test_strdup(void) {
    expect("strdup_copy",
        "package main;\n"
        "char *strdup(char *s);\n"
        "int main(void) {\n"
        "  char s[4];\n"
        "  s[0] = 'a'; s[1] = 'b'; s[2] = 0;\n"
        "  char *p = strdup(s);\n"
        "  if (!p) return 1;\n"
        "  if (p[0] != 'a' || p[1] != 'b' || p[2]) return 2;\n"
        "  p[0] = 'z';\n"
        "  if (s[0] != 'a') return 3;\n"
        "  return p[0] == 'z' ? 7 : 4;\n"
        "}\n", 7);
    expect("strndup_cut",
        "package main;\n"
        "char *strndup(char *s, unsigned long n);\n"
        "int main(void) {\n"
        "  char *p = strndup(\"hello\", 3);\n"
        "  if (!p) return 1;\n"
        "  if (p[0] != 'h' || p[1] != 'e' || p[2] != 'l' || p[3]) return 2;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("strdup_user",
        "package main;\n"
        "char *strdup(char *s) { return s ? 0 : 0; }\n"
        "int main(void) { return strdup(\"a\") ? 1 : 7; }\n", 7);
}

static void test_io(void) {
    /* Darwin open flags: O_RDWR|O_CREAT|O_TRUNC = 0x602.  EBADF is 9. */
    expect("io_roundtrip",
        "package main;\n"
        "long open(char *path, long flags, long mode);\n"
        "long write(long fd, char *buf, long n);\n"
        "long read(long fd, char *buf, long n);\n"
        "long lseek(long fd, long off, long whence);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  long fd = open(\"/tmp/fakecc_io_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  char *msg = \"ab\";\n"
        "  if (write(fd, msg, 2) != 2) return 2;\n"
        "  if (lseek(fd, 0, 0) != 0) return 3;\n"
        "  char buf[4];\n"
        "  buf[0] = 0; buf[1] = 0;\n"
        "  if (read(fd, buf, 2) != 2) return 4;\n"
        "  if (buf[0] != 'a' || buf[1] != 'b') return 5;\n"
        "  if (close(fd) != 0) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("write_badfd",
        "package main;\n"
        "long write(long fd, char *buf, long n);\n"
        "int main(void) {\n"
        "  char *p = \"x\";\n"
        "  return write(-1, p, 1) == -9 ? 7 : 1;\n"
        "}\n", 7);
    expect("write_user",
        "package main;\n"
        "long write(long fd, char *buf, long n) { return n ? 3 : 3; }\n"
        "int main(void) { return write(1, 0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("unlink_chmod",
        "package main;\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long chmod(char *path, long mode);\n"
        "long unlink(char *path);\n"
        "int main(void) {\n"
        "  char *path = \"/tmp/fakecc_io_un\";\n"
        "  long fd = open(path, 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  if (close(fd) != 0) return 2;\n"
        "  if (chmod(path, 384) != 0) return 3;\n"
        "  if (unlink(path) != 0) return 4;\n"
        "  if (open(path, 0, 0) != -2) return 5;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("unlink_user",
        "package main;\n"
        "long unlink(char *path) { return path ? 3 : 3; }\n"
        "int main(void) { return unlink(\"x\") == 3 ? 7 : 1; }\n", 7);
    expect("errno_write",
        "package main;\n"
        "int errno;\n"
        "long write(long fd, char *buf, long n);\n"
        "int main(void) {\n"
        "  char *p = \"x\";\n"
        "  if (write(-1, p, 1) != -9) return 1;\n"
        "  if (errno != 9) return 2;\n"
        "  if (write(1, p, 0) != 0) return 3;\n"
        "  if (errno != 0) return 4;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getpid_stable",
        "package main;\n"
        "long getpid(void);\n"
        "int main(void) {\n"
        "  long a = getpid();\n"
        "  if (a <= 0) return 1;\n"
        "  if (getpid() != a) return 2;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getpid_user",
        "package main;\n"
        "long getpid(void) { return 4; }\n"
        "int main(void) { return getpid() == 4 ? 7 : 1; }\n", 7);
    expect("mkdir_rmdir",
        "package main;\n"
        "int errno;\n"
        "long mkdir(char *path, long mode);\n"
        "long rmdir(char *path);\n"
        "int main(void) {\n"
        "  char *path = \"/tmp/fakecc_mkdir_rt\";\n"
        "  rmdir(path);\n"
        "  if (mkdir(path, 448) != 0) return 1;\n"
        "  if (mkdir(path, 448) != -17) return 2;\n"
        "  if (errno != 17) return 3;\n"
        "  if (rmdir(path) != 0) return 4;\n"
        "  if (rmdir(path) != -2) return 5;\n"
        "  if (errno != 2) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("mkdir_user",
        "package main;\n"
        "long mkdir(char *path, long mode) { return path ? 3 : 3; }\n"
        "int main(void) { return mkdir(\"x\", 0) == 3 ? 7 : 1; }\n", 7);
    expect("access_file",
        "package main;\n"
        "int errno;\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long unlink(char *path);\n"
        "long access(char *path, long mode);\n"
        "int main(void) {\n"
        "  char *path = \"/tmp/fakecc_access_rt\";\n"
        "  unlink(path);\n"
        "  long fd = open(path, 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  if (close(fd) != 0) return 2;\n"
        "  if (access(path, 0) != 0) return 3;\n"
        "  if (access(path, 4) != 0) return 4;\n"
        "  if (unlink(path) != 0) return 5;\n"
        "  if (access(path, 0) != -2) return 6;\n"
        "  if (errno != 2) return 8;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("access_user",
        "package main;\n"
        "long access(char *path, long mode) { return path ? 3 : 3; }\n"
        "int main(void) { return access(\"x\", 0) == 3 ? 7 : 1; }\n", 7);
    expect("chdir_rename",
        "package main;\n"
        "int errno;\n"
        "long mkdir(char *path, long mode);\n"
        "long rmdir(char *path);\n"
        "long chdir(char *path);\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long access(char *path, long mode);\n"
        "long unlink(char *path);\n"
        "long rename(char *old, char *neu);\n"
        "int main(void) {\n"
        "  char *dir = \"/tmp/fakecc_chdir_rt\";\n"
        "  char *abs = \"/tmp/fakecc_chdir_rt/note\";\n"
        "  char *neu = \"/tmp/fakecc_chdir_rt/ren\";\n"
        "  if (chdir(\"/tmp/fakecc_chdir_missing\") != -2) return 1;\n"
        "  if (errno != 2) return 2;\n"
        "  unlink(abs);\n"
        "  unlink(neu);\n"
        "  rmdir(dir);\n"
        "  if (mkdir(dir, 448) != 0) return 3;\n"
        "  if (chdir(dir) != 0) return 4;\n"
        "  long fd = open(\"note\", 0x602, 420);\n"
        "  if (fd < 0) return 5;\n"
        "  if (close(fd) != 0) return 6;\n"
        "  if (access(abs, 0) != 0) return 8;\n"
        "  if (rename(abs, neu) != 0) return 9;\n"
        "  if (access(neu, 0) != 0) return 10;\n"
        "  if (chdir(\"/tmp\") != 0) return 11;\n"
        "  if (unlink(neu) != 0) return 12;\n"
        "  if (rmdir(dir) != 0) return 13;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("chdir_user",
        "package main;\n"
        "long chdir(char *path) { return path ? 3 : 3; }\n"
        "int main(void) { return chdir(\"x\") == 3 ? 7 : 1; }\n", 7);
    expect("symlink_readlink",
        "package main;\n"
        "int errno;\n"
        "long symlink(char *target, char *path);\n"
        "long readlink(char *path, char *buf, long n);\n"
        "long unlink(char *path);\n"
        "int main(void) {\n"
        "  char *link = \"/tmp/fakecc_sym_rt\";\n"
        "  unlink(link);\n"
        "  if (symlink(\"target\", link) != 0) return 1;\n"
        "  char buf[8];\n"
        "  int i;\n"
        "  for (i = 0; i < 8; i++) buf[i] = 1;\n"
        "  if (readlink(link, buf, 8) != 6) return 2;\n"
        "  if (buf[0] != 't' || buf[5] != 't' || buf[6] != 1) return 3;\n"
        "  if (unlink(link) != 0) return 4;\n"
        "  if (readlink(link, buf, 8) != -2) return 5;\n"
        "  if (errno != 2) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("symlink_user",
        "package main;\n"
        "long symlink(char *target, char *path) { return target ? 3 : 3; }\n"
        "int main(void) { return symlink(\"a\", \"b\") == 3 ? 7 : 1; }\n", 7);
    expect("hard_link",
        "package main;\n"
        "int errno;\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long write(long fd, char *buf, long n);\n"
        "long read(long fd, char *buf, long n);\n"
        "long link(char *old, char *neu);\n"
        "long unlink(char *path);\n"
        "long access(char *path, long mode);\n"
        "int main(void) {\n"
        "  char *a = \"/tmp/fakecc_link_a\";\n"
        "  char *b = \"/tmp/fakecc_link_b\";\n"
        "  unlink(a);\n"
        "  unlink(b);\n"
        "  long fd = open(a, 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  char *msg = \"xy\";\n"
        "  if (write(fd, msg, 2) != 2) return 2;\n"
        "  if (close(fd) != 0) return 3;\n"
        "  if (link(a, b) != 0) return 4;\n"
        "  fd = open(b, 0, 0);\n"
        "  if (fd < 0) return 5;\n"
        "  char buf[4];\n"
        "  buf[0] = 0; buf[1] = 0;\n"
        "  if (read(fd, buf, 2) != 2) return 6;\n"
        "  if (buf[0] != 'x' || buf[1] != 'y') return 8;\n"
        "  if (close(fd) != 0) return 9;\n"
        "  if (unlink(a) != 0) return 10;\n"
        "  if (access(b, 0) != 0) return 11;\n"
        "  if (unlink(b) != 0) return 12;\n"
        "  if (link(a, b) != -2) return 13;\n"
        "  if (errno != 2) return 14;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("link_user",
        "package main;\n"
        "long link(char *old, char *neu) { return old ? 3 : 3; }\n"
        "int main(void) { return link(\"a\", \"b\") == 3 ? 7 : 1; }\n", 7);
    expect("getcwd_tmp",
        "package main;\n"
        "int errno;\n"
        "long chdir(char *path);\n"
        "char *getcwd(char *buf, unsigned long n);\n"
        "int main(void) {\n"
        "  if (chdir(\"/tmp\") != 0) return 1;\n"
        "  char buf[64];\n"
        "  char *p = getcwd(buf, 64);\n"
        "  if (p != buf || buf[0] != '/') return 2;\n"
        "  int n = 0;\n"
        "  while (buf[n]) n++;\n"
        "  if (n < 3) return 3;\n"
        "  if (buf[n - 3] != 't' || buf[n - 2] != 'm' || buf[n - 1] != 'p')\n"
        "    return 4;\n"
        "  if (getcwd(buf, 0)) return 5;\n"
        "  if (errno != 22) return 6;\n"
        "  char tiny[1];\n"
        "  if (getcwd(tiny, 1)) return 8;\n"
        "  if (errno != 34) return 9;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getcwd_user",
        "package main;\n"
        "char *getcwd(char *buf, unsigned long n) { return buf ? 0 : 0; }\n"
        "int main(void) {\n"
        "  char b[4];\n"
        "  return getcwd(b, 4) ? 1 : 7;\n"
        "}\n", 7);
    expect("dup_offset",
        "package main;\n"
        "int errno;\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long write(long fd, char *buf, long n);\n"
        "long read(long fd, char *buf, long n);\n"
        "long lseek(long fd, long off, long whence);\n"
        "long dup(long fd);\n"
        "long dup2(long fd, long neu);\n"
        "long unlink(char *path);\n"
        "int main(void) {\n"
        "  char *path = \"/tmp/fakecc_dup_rt\";\n"
        "  unlink(path);\n"
        "  long fd = open(path, 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  char *msg = \"xy\";\n"
        "  if (write(fd, msg, 2) != 2) return 2;\n"
        "  long d = dup(fd);\n"
        "  if (d < 0) return 3;\n"
        "  if (lseek(d, 0, 0) != 0) return 4;\n"
        "  char buf[4];\n"
        "  buf[0] = 0; buf[1] = 0;\n"
        "  if (read(fd, buf, 2) != 2) return 5;\n"
        "  if (buf[0] != 'x' || buf[1] != 'y') return 6;\n"
        "  if (dup(-1) != -9 || errno != 9) return 8;\n"
        "  if (dup2(fd, 80) != 80) return 9;\n"
        "  if (close(fd) != 0 || close(d) != 0 || close(80) != 0) return 10;\n"
        "  if (unlink(path) != 0) return 11;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("dup_user",
        "package main;\n"
        "long dup(long fd) { return fd ? 3 : 3; }\n"
        "int main(void) { return dup(1) == 3 ? 7 : 1; }\n", 7);
    expect("pipe_bytes",
        "package main;\n"
        "long pipe(int *fds);\n"
        "long write(long fd, char *buf, long n);\n"
        "long read(long fd, char *buf, long n);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  int fds[2];\n"
        "  if (pipe(fds) != 0) return 1;\n"
        "  if (fds[0] < 0 || fds[1] < 0) return 2;\n"
        "  char *msg = \"ab\";\n"
        "  if (write(fds[1], msg, 2) != 2) return 3;\n"
        "  char buf[4];\n"
        "  buf[0] = 0; buf[1] = 0;\n"
        "  if (read(fds[0], buf, 2) != 2) return 4;\n"
        "  if (buf[0] != 'a' || buf[1] != 'b') return 5;\n"
        "  if (close(fds[0]) != 0 || close(fds[1]) != 0) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("pipe_user",
        "package main;\n"
        "long pipe(int *fds) { return fds ? 3 : 3; }\n"
        "int main(void) {\n"
        "  int fds[2];\n"
        "  return pipe(fds) == 3 ? 7 : 1;\n"
        "}\n", 7);
    expect("truncate_len",
        "package main;\n"
        "int errno;\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long write(long fd, char *buf, long n);\n"
        "long lseek(long fd, long off, long whence);\n"
        "long ftruncate(long fd, long len);\n"
        "long truncate(char *path, long len);\n"
        "long unlink(char *path);\n"
        "int main(void) {\n"
        "  char *path = \"/tmp/fakecc_trunc_rt\";\n"
        "  unlink(path);\n"
        "  long fd = open(path, 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  char *msg = \"abcd\";\n"
        "  if (write(fd, msg, 4) != 4) return 2;\n"
        "  if (ftruncate(fd, 2) != 0) return 3;\n"
        "  if (lseek(fd, 0, 2) != 2) return 4;\n"
        "  if (truncate(path, 1) != 0) return 5;\n"
        "  if (lseek(fd, 0, 2) != 1) return 6;\n"
        "  if (ftruncate(-1, 0) != -9 || errno != 9) return 8;\n"
        "  if (close(fd) != 0 || unlink(path) != 0) return 9;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("ftruncate_user",
        "package main;\n"
        "long ftruncate(long fd, long len) { return fd ? 3 : 3; }\n"
        "int main(void) { return ftruncate(1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("pread_keeps_offset",
        "package main;\n"
        "int errno;\n"
        "long open(char *path, long flags, long mode);\n"
        "long close(long fd);\n"
        "long write(long fd, char *buf, long n);\n"
        "long read(long fd, char *buf, long n);\n"
        "long pread(long fd, char *buf, long n, long off);\n"
        "long pwrite(long fd, char *buf, long n, long off);\n"
        "long lseek(long fd, long off, long whence);\n"
        "long fsync(long fd);\n"
        "long unlink(char *path);\n"
        "int main(void) {\n"
        "  char *path = \"/tmp/fakecc_pread_rt\";\n"
        "  unlink(path);\n"
        "  long fd = open(path, 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  char *msg = \"abcd\";\n"
        "  if (write(fd, msg, 4) != 4) return 2;\n"
        "  char buf[4];\n"
        "  buf[0] = 0; buf[1] = 0;\n"
        "  if (pread(fd, buf, 2, 1) != 2) return 3;\n"
        "  if (buf[0] != 'b' || buf[1] != 'c') return 4;\n"
        "  if (lseek(fd, 0, 1) != 4) return 5;\n"
        "  char *z = \"Z\";\n"
        "  if (pwrite(fd, z, 1, 0) != 1) return 6;\n"
        "  if (lseek(fd, 0, 1) != 4) return 8;\n"
        "  if (fsync(fd) != 0) return 9;\n"
        "  if (pread(-1, buf, 1, 0) != -9 || errno != 9) return 10;\n"
        "  if (close(fd) != 0 || unlink(path) != 0) return 11;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("pread_user",
        "package main;\n"
        "long pread(long fd, char *buf, long n, long off) { return fd ? 3 : 3; }\n"
        "int main(void) { return pread(1, 0, 0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("ids_stable",
        "package main;\n"
        "long getuid(void);\n"
        "long geteuid(void);\n"
        "long getgid(void);\n"
        "long getegid(void);\n"
        "long getppid(void);\n"
        "int main(void) {\n"
        "  long u = getuid();\n"
        "  if (u < 0 || geteuid() != u) return 1;\n"
        "  long g = getgid();\n"
        "  if (g < 0 || getegid() != g) return 2;\n"
        "  if (getppid() < 0) return 3;\n"
        "  if (getuid() != u || getgid() != g) return 4;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getuid_user",
        "package main;\n"
        "long getuid(void) { return 4; }\n"
        "int main(void) { return getuid() == 4 ? 7 : 1; }\n", 7);
    expect("umask_roundtrip",
        "package main;\n"
        "long umask(long mask);\n"
        "int main(void) {\n"
        "  long old = umask(18);\n"
        "  if (old < 0) return 1;\n"
        "  if (umask(old) != 18) return 2;\n"
        "  if (umask(old) != old) return 3;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("umask_user",
        "package main;\n"
        "long umask(long mask) { return mask ? 3 : 3; }\n"
        "int main(void) { return umask(18) == 3 ? 7 : 1; }\n", 7);
    expect("gettimeofday_now",
        "package main;\n"
        "long gettimeofday(long *tv, long *tz);\n"
        "int main(void) {\n"
        "  long tv[2];\n"
        "  tv[0] = 0; tv[1] = -1;\n"
        "  if (gettimeofday(tv, 0) != 0) return 1;\n"
        "  if (tv[0] < 1700000000 || tv[0] > 2000000000) return 2;\n"
        "  long usec = tv[1] & 0xffffffff;\n"
        "  if (usec < 0 || usec >= 1000000) return 3;\n"
        "  if (gettimeofday(0, 0) != 0) return 4;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("gettimeofday_user",
        "package main;\n"
        "long gettimeofday(long *tv, long *tz) { return tv ? 3 : 3; }\n"
        "int main(void) { return gettimeofday(0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("getentropy_bytes",
        "package main;\n"
        "int errno;\n"
        "long getentropy(char *buf, long n);\n"
        "long issetugid(void);\n"
        "int main(void) {\n"
        "  char b[8];\n"
        "  int i;\n"
        "  for (i = 0; i < 8; i++) b[i] = 0;\n"
        "  if (getentropy(b, 8) != 0) return 1;\n"
        "  int any = 0;\n"
        "  for (i = 0; i < 8; i++) if (b[i]) any = 1;\n"
        "  if (!any) return 2;\n"
        "  if (getentropy(b, 257) >= 0 || errno == 0) return 3;\n"
        "  if (getentropy(b, 0) != 0 || errno != 0) return 4;\n"
        "  long ug = issetugid();\n"
        "  if (ug != 0 && ug != 1) return 5;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getentropy_user",
        "package main;\n"
        "long getentropy(char *buf, long n) { return n ? 3 : 3; }\n"
        "int main(void) { return getentropy(0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("rlimit_nofile",
        "package main;\n"
        "int errno;\n"
        "long getrlimit(long res, long *rl);\n"
        "long setrlimit(long res, long *rl);\n"
        "int main(void) {\n"
        "  long rl[2];\n"
        "  if (getrlimit(8, rl) != 0) return 1;\n"
        "  if (rl[0] <= 0) return 2;\n"
        "  if (rl[1] != -1 && rl[1] < rl[0]) return 3;\n"
        "  if (getrlimit(99, rl) >= 0 || errno != 22) return 4;\n"
        "  if (getrlimit(8, rl) != 0 || errno != 0) return 5;\n"
        "  long pair[2];\n"
        "  pair[0] = rl[0];\n"
        "  pair[1] = rl[1];\n"
        "  if (setrlimit(8, pair) != 0 || errno != 0) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("rlimit_user",
        "package main;\n"
        "long getrlimit(long res, long *rl) { return res ? 3 : 3; }\n"
        "int main(void) { return getrlimit(8, 0) == 3 ? 7 : 1; }\n", 7);
    expect("pgrp_ids",
        "package main;\n"
        "int errno;\n"
        "long getpgrp(void);\n"
        "long getpgid(long pid);\n"
        "long getsid(long pid);\n"
        "long getpid(void);\n"
        "int main(void) {\n"
        "  long g = getpgrp();\n"
        "  if (g <= 0) return 1;\n"
        "  if (getpgid(0) != g) return 2;\n"
        "  if (getpgid(getpid()) != g) return 3;\n"
        "  if (getsid(0) <= 0) return 4;\n"
        "  if (getpgid(-1) != -3 || errno != 3) return 5;\n"
        "  if (getpgrp() != g || errno != 0) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("pgrp_user",
        "package main;\n"
        "long getpgrp(void) { return 3; }\n"
        "int main(void) { return getpgrp() == 3 ? 7 : 1; }\n", 7);
    expect("fchdir_tmp",
        "package main;\n"
        "int errno;\n"
        "long fchdir(long fd);\n"
        "long getcwd(char *b, long n);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  char a[1040];\n"
        "  char b[1040];\n"
        "  if (!getcwd(a, 1040)) return 1;\n"
        "  long old = open(\".\", 0, 0);\n"
        "  if (old < 0) return 2;\n"
        "  long fd = open(\"/tmp\", 0, 0);\n"
        "  if (fd < 0) return 3;\n"
        "  if (fchdir(fd) != 0) return 4;\n"
        "  if (!getcwd(b, 1040)) return 5;\n"
        "  int i = 0;\n"
        "  while (b[i]) i++;\n"
        "  if (i < 3 || b[i - 3] != 't' || b[i - 2] != 'm' || b[i - 1] != 'p') return 6;\n"
        "  if (fchdir(-1) != -9 || errno != 9) return 8;\n"
        "  if (fchdir(old) != 0 || errno != 0) return 9;\n"
        "  if (!getcwd(b, 1040)) return 10;\n"
        "  i = 0;\n"
        "  while (a[i] && a[i] == b[i]) i++;\n"
        "  if (a[i] || b[i]) return 11;\n"
        "  close(old);\n"
        "  close(fd);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("fchdir_user",
        "package main;\n"
        "long fchdir(long fd) { return fd ? 3 : 3; }\n"
        "int main(void) { return fchdir(1) == 3 ? 7 : 1; }\n", 7);
    expect("flock_file",
        "package main;\n"
        "int errno;\n"
        "long flock(long fd, long op);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_flock_rt\");\n"
        "  long fd = open(\"/tmp/fakecc_flock_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  if (flock(fd, 2) != 0) return 2;\n"
        "  if (flock(fd, 1) != 0) return 3;\n"
        "  if (flock(fd, 8) != 0) return 4;\n"
        "  if (flock(-1, 2) != -9 || errno != 9) return 5;\n"
        "  if (flock(fd, 2) != 0 || errno != 0) return 6;\n"
        "  flock(fd, 8);\n"
        "  close(fd);\n"
        "  unlink(\"/tmp/fakecc_flock_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("flock_user",
        "package main;\n"
        "long flock(long fd, long op) { return op ? 3 : 3; }\n"
        "int main(void) { return flock(1, 2) == 3 ? 7 : 1; }\n", 7);
    expect("getgroups_list",
        "package main;\n"
        "int errno;\n"
        "long getgroups(long n, int *g);\n"
        "long getdtablesize(void);\n"
        "int main(void) {\n"
        "  long n = getgroups(0, 0);\n"
        "  if (n <= 0 || n > 64) return 1;\n"
        "  int g[64];\n"
        "  int i;\n"
        "  for (i = 0; i < 64; i++) g[i] = -1;\n"
        "  if (n > 1 && (getgroups(1, g) >= 0 || errno != 22)) return 2;\n"
        "  if (getgroups(n, g) != n || errno != 0) return 3;\n"
        "  if (g[0] < 0) return 4;\n"
        "  if (getdtablesize() <= 0) return 5;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getgroups_user",
        "package main;\n"
        "long getgroups(long n, int *g) { return n ? 3 : 3; }\n"
        "int main(void) { return getgroups(1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("mkfifo_path",
        "package main;\n"
        "int errno;\n"
        "long mkfifo(char *path, long mode);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_fifo_rt\");\n"
        "  if (mkfifo(\"/tmp/fakecc_fifo_rt\", 420) != 0) return 1;\n"
        "  if (mkfifo(\"/tmp/fakecc_fifo_rt\", 420) != -17 || errno != 17) return 2;\n"
        "  if (mkfifo(\"/tmp/no_such_dir_fakecc/x\", 420) != -2 || errno != 2) return 3;\n"
        "  if (unlink(\"/tmp/fakecc_fifo_rt\") != 0 || errno != 0) return 4;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("mkfifo_user",
        "package main;\n"
        "long mkfifo(char *path, long mode) { return mode ? 3 : 3; }\n"
        "int main(void) { return mkfifo(0, 420) == 3 ? 7 : 1; }\n", 7);
    expect("pathconf_tmp",
        "package main;\n"
        "int errno;\n"
        "long pathconf(char *path, long name);\n"
        "long fpathconf(long fd, long name);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  if (pathconf(\"/tmp\", 4) != 255) return 1;\n"
        "  if (pathconf(\"/tmp\", 5) != 1024) return 2;\n"
        "  if (pathconf(\"/tmp/no_such_fakecc_pc\", 4) != -2 || errno != 2) return 3;\n"
        "  if (pathconf(\"/tmp\", 99) != -22 || errno != 22) return 4;\n"
        "  long fd = open(\"/tmp\", 0, 0);\n"
        "  if (fd < 0) return 5;\n"
        "  if (fpathconf(fd, 4) != 255 || errno != 0) return 6;\n"
        "  if (fpathconf(-1, 4) != -9 || errno != 9) return 8;\n"
        "  close(fd);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("pathconf_user",
        "package main;\n"
        "long pathconf(char *path, long name) { return name ? 3 : 3; }\n"
        "int main(void) { return pathconf(0, 4) == 3 ? 7 : 1; }\n", 7);
    expect("utimes_file",
        "package main;\n"
        "int errno;\n"
        "long utimes(char *path, long *tv);\n"
        "long futimes(long fd, long *tv);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_utimes_rt\");\n"
        "  long fd = open(\"/tmp/fakecc_utimes_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  close(fd);\n"
        "  if (utimes(\"/tmp/fakecc_utimes_rt\", 0) != 0) return 2;\n"
        "  long tv[4];\n"
        "  tv[0] = 1700000000; tv[1] = 0;\n"
        "  tv[2] = 1700000001; tv[3] = 0;\n"
        "  if (utimes(\"/tmp/fakecc_utimes_rt\", tv) != 0) return 3;\n"
        "  if (utimes(\"/tmp/no_such_fakecc_ut\", 0) != -2 || errno != 2) return 4;\n"
        "  fd = open(\"/tmp/fakecc_utimes_rt\", 2, 0);\n"
        "  if (fd < 0) return 5;\n"
        "  if (futimes(fd, tv) != 0 || errno != 0) return 6;\n"
        "  if (futimes(-1, 0) != -9 || errno != 9) return 8;\n"
        "  close(fd);\n"
        "  unlink(\"/tmp/fakecc_utimes_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("utimes_user",
        "package main;\n"
        "long utimes(char *path, long *tv) { return path ? 3 : 3; }\n"
        "int main(void) { return utimes(0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("chown_self",
        "package main;\n"
        "int errno;\n"
        "long chown(char *path, long uid, long gid);\n"
        "long fchown(long fd, long uid, long gid);\n"
        "long lchown(char *path, long uid, long gid);\n"
        "long getuid(void);\n"
        "long getgid(void);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "long symlink(char *target, char *link);\n"
        "int main(void) {\n"
        "  long uid = getuid();\n"
        "  long gid = getgid();\n"
        "  if (uid < 0 || gid < 0) return 1;\n"
        "  unlink(\"/tmp/fakecc_chown_rt\");\n"
        "  unlink(\"/tmp/fakecc_chown_ln\");\n"
        "  long fd = open(\"/tmp/fakecc_chown_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 2;\n"
        "  if (chown(\"/tmp/fakecc_chown_rt\", uid, gid) != 0) return 3;\n"
        "  if (chown(\"/tmp/fakecc_chown_rt\", -1, -1) != 0) return 4;\n"
        "  if (chown(\"/tmp/no_such_fakecc_ch\", uid, gid) != -2 || errno != 2) return 5;\n"
        "  if (fchown(fd, uid, gid) != 0 || errno != 0) return 6;\n"
        "  if (fchown(-1, uid, gid) != -9 || errno != 9) return 8;\n"
        "  if (symlink(\"/tmp/fakecc_chown_rt\", \"/tmp/fakecc_chown_ln\") != 0) return 9;\n"
        "  if (lchown(\"/tmp/fakecc_chown_ln\", uid, gid) != 0 || errno != 0) return 10;\n"
        "  close(fd);\n"
        "  unlink(\"/tmp/fakecc_chown_ln\");\n"
        "  unlink(\"/tmp/fakecc_chown_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("chown_user",
        "package main;\n"
        "long chown(char *path, long uid, long gid) { return uid ? 3 : 3; }\n"
        "int main(void) { return chown(0, 1, 1) == 3 ? 7 : 1; }\n", 7);
    expect("socketpair_stream",
        "package main;\n"
        "int errno;\n"
        "long socketpair(long domain, long type, long proto, int *sv);\n"
        "long read(long fd, char *buf, long n);\n"
        "long write(long fd, char *buf, long n);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  int sv[2];\n"
        "  sv[0] = -1; sv[1] = -1;\n"
        "  if (socketpair(99, 1, 0, sv) != -47 || errno != 47) return 1;\n"
        "  if (socketpair(1, 1, 0, sv) != 0 || errno != 0) return 2;\n"
        "  if (sv[0] < 0 || sv[1] < 0 || sv[0] == sv[1]) return 3;\n"
        "  char c = 42;\n"
        "  if (write(sv[0], &c, 1) != 1) return 4;\n"
        "  char d = 0;\n"
        "  if (read(sv[1], &d, 1) != 1 || d != 42) return 5;\n"
        "  close(sv[0]);\n"
        "  close(sv[1]);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("socketpair_user",
        "package main;\n"
        "long socketpair(long domain, long type, long proto, int *sv) { return domain ? 3 : 3; }\n"
        "int main(void) { return socketpair(1, 1, 0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("readv_file",
        "package main;\n"
        "int errno;\n"
        "long readv(long fd, long *iov, long n);\n"
        "long writev(long fd, long *iov, long n);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "long lseek(long fd, long off, long whence);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_readv_rt\");\n"
        "  long fd = open(\"/tmp/fakecc_readv_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  char a[2]; a[0] = 'a'; a[1] = 'b';\n"
        "  char b[2]; b[0] = 'c'; b[1] = 'd';\n"
        "  long iov[4];\n"
        "  iov[0] = (long)a; iov[1] = 2;\n"
        "  iov[2] = (long)b; iov[3] = 2;\n"
        "  if (writev(fd, iov, 2) != 4) return 2;\n"
        "  if (lseek(fd, 0, 0) != 0) return 3;\n"
        "  char c[2]; c[0] = 0; c[1] = 0;\n"
        "  char d[2]; d[0] = 0; d[1] = 0;\n"
        "  long riov[4];\n"
        "  riov[0] = (long)c; riov[1] = 2;\n"
        "  riov[2] = (long)d; riov[3] = 2;\n"
        "  if (readv(fd, riov, 2) != 4) return 4;\n"
        "  if (c[0] != 'a' || c[1] != 'b' || d[0] != 'c' || d[1] != 'd') return 5;\n"
        "  if (readv(-1, riov, 2) != -9 || errno != 9) return 6;\n"
        "  if (lseek(fd, 0, 0) != 0) return 8;\n"
        "  if (readv(fd, riov, 2) != 4 || errno != 0) return 9;\n"
        "  close(fd);\n"
        "  unlink(\"/tmp/fakecc_readv_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("readv_user",
        "package main;\n"
        "long readv(long fd, long *iov, long n) { return n ? 3 : 3; }\n"
        "int main(void) { return readv(0, 0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("socket_unix",
        "package main;\n"
        "int errno;\n"
        "long socket(long domain, long type, long proto);\n"
        "long shutdown(long fd, long how);\n"
        "long socketpair(long domain, long type, long proto, int *sv);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  long fd = socket(1, 1, 0);\n"
        "  if (fd < 0) return 1;\n"
        "  if (socket(99, 1, 0) != -47 || errno != 47) return 2;\n"
        "  int sv[2];\n"
        "  if (socketpair(1, 1, 0, sv) != 0 || errno != 0) return 3;\n"
        "  if (shutdown(sv[0], 2) != 0) return 4;\n"
        "  if (shutdown(-1, 2) != -9 || errno != 9) return 5;\n"
        "  close(fd);\n"
        "  close(sv[0]);\n"
        "  close(sv[1]);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("socket_user",
        "package main;\n"
        "long socket(long domain, long type, long proto) { return domain ? 3 : 3; }\n"
        "int main(void) { return socket(1, 1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("sockopt_type",
        "package main;\n"
        "int errno;\n"
        "long socket(long domain, long type, long proto);\n"
        "long getsockopt(long fd, long level, long name, int *val, int *len);\n"
        "long setsockopt(long fd, long level, long name, int *val, long len);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  long fd = socket(1, 1, 0);\n"
        "  if (fd < 0) return 1;\n"
        "  int typ = -1;\n"
        "  int len = 4;\n"
        "  if (getsockopt(fd, 0xffff, 0x1008, &typ, &len) != 0) return 2;\n"
        "  if (typ != 1 || len != 4) return 3;\n"
        "  int one = 1;\n"
        "  if (setsockopt(fd, 0xffff, 4, &one, 4) != 0) return 4;\n"
        "  if (getsockopt(-1, 0xffff, 0x1008, &typ, &len) != -9 || errno != 9) return 5;\n"
        "  len = 4;\n"
        "  if (getsockopt(fd, 0xffff, 0x1008, &typ, &len) != 0 || errno != 0) return 6;\n"
        "  close(fd);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("sockopt_user",
        "package main;\n"
        "long getsockopt(long fd, long level, long name, int *val, int *len) { return name ? 3 : 3; }\n"
        "int main(void) { return getsockopt(0, 0, 1, 0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("bind_unix",
        "package main;\n"
        "int errno;\n"
        "long socket(long domain, long type, long proto);\n"
        "long bind(long fd, char *addr, long len);\n"
        "long listen(long fd, long backlog);\n"
        "long getsockname(long fd, char *addr, int *len);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_bind_rt\");\n"
        "  long fd = socket(1, 1, 0);\n"
        "  if (fd < 0) return 1;\n"
        "  char *s = \"/tmp/fakecc_bind_rt\";\n"
        "  int n = 0;\n"
        "  while (s[n]) n++;\n"
        "  char addr[108];\n"
        "  int i;\n"
        "  for (i = 0; i < 108; i++) addr[i] = 0;\n"
        "  addr[0] = (char)(2 + n);\n"
        "  addr[1] = 1;\n"
        "  for (i = 0; i < n; i++) addr[2 + i] = s[i];\n"
        "  if (bind(fd, addr, 2 + n) != 0) return 2;\n"
        "  if (listen(fd, 1) != 0) return 3;\n"
        "  char got[108];\n"
        "  int glen = 108;\n"
        "  if (getsockname(fd, got, &glen) != 0) return 4;\n"
        "  if (got[1] != 1 || glen < 2 + n) return 5;\n"
        "  for (i = 0; i < n; i++) if (got[2 + i] != s[i]) return 6;\n"
        "  long fd2 = socket(1, 1, 0);\n"
        "  if (fd2 < 0) return 8;\n"
        "  if (bind(fd2, addr, 2 + n) != -48 || errno != 48) return 9;\n"
        "  if (bind(-1, addr, 2 + n) != -9 || errno != 9) return 10;\n"
        "  glen = 108;\n"
        "  if (getsockname(fd, got, &glen) != 0 || errno != 0) return 11;\n"
        "  close(fd);\n"
        "  close(fd2);\n"
        "  unlink(\"/tmp/fakecc_bind_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("bind_user",
        "package main;\n"
        "long bind(long fd, char *addr, long len) { return len ? 3 : 3; }\n"
        "int main(void) { return bind(0, 0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("accept_unix",
        "package main;\n"
        "int errno;\n"
        "long socket(long domain, long type, long proto);\n"
        "long bind(long fd, char *addr, long len);\n"
        "long listen(long fd, long backlog);\n"
        "long connect(long fd, char *addr, long len);\n"
        "long accept(long fd, char *addr, int *len);\n"
        "long read(long fd, char *buf, long n);\n"
        "long write(long fd, char *buf, long n);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_acc_rt\");\n"
        "  long srv = socket(1, 1, 0);\n"
        "  if (srv < 0) return 1;\n"
        "  char *s = \"/tmp/fakecc_acc_rt\";\n"
        "  int n = 0;\n"
        "  while (s[n]) n++;\n"
        "  char addr[108];\n"
        "  int i;\n"
        "  for (i = 0; i < 108; i++) addr[i] = 0;\n"
        "  addr[0] = (char)(2 + n);\n"
        "  addr[1] = 1;\n"
        "  for (i = 0; i < n; i++) addr[2 + i] = s[i];\n"
        "  if (bind(srv, addr, 2 + n) != 0) return 2;\n"
        "  if (listen(srv, 1) != 0) return 3;\n"
        "  long cli = socket(1, 1, 0);\n"
        "  if (cli < 0) return 4;\n"
        "  if (connect(cli, addr, 2 + n) != 0) return 5;\n"
        "  char peer[108];\n"
        "  int plen = 108;\n"
        "  long acc = accept(srv, peer, &plen);\n"
        "  if (acc < 0) return 6;\n"
        "  char c = 42;\n"
        "  if (write(cli, &c, 1) != 1) return 8;\n"
        "  char d = 0;\n"
        "  if (read(acc, &d, 1) != 1 || d != 42) return 9;\n"
        "  if (connect(-1, addr, 2 + n) != -9 || errno != 9) return 10;\n"
        "  if (accept(-1, peer, &plen) != -9 || errno != 9) return 11;\n"
        "  close(acc);\n"
        "  close(cli);\n"
        "  close(srv);\n"
        "  unlink(\"/tmp/fakecc_acc_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("connect_user",
        "package main;\n"
        "long connect(long fd, char *addr, long len) { return len ? 3 : 3; }\n"
        "int main(void) { return connect(0, 0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("sendto_pair",
        "package main;\n"
        "int errno;\n"
        "long socketpair(long domain, long type, long proto, int *sv);\n"
        "long sendto(long fd, char *buf, long n, long flags, char *addr, long alen);\n"
        "long recvfrom(long fd, char *buf, long n, long flags, char *addr, int *alen);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  int sv[2];\n"
        "  if (socketpair(1, 1, 0, sv) != 0) return 1;\n"
        "  char c = 42;\n"
        "  if (sendto(sv[0], &c, 1, 0, 0, 0) != 1) return 2;\n"
        "  char d = 0;\n"
        "  if (recvfrom(sv[1], &d, 1, 0, 0, 0) != 1 || d != 42) return 3;\n"
        "  if (sendto(-1, &c, 1, 0, 0, 0) != -9 || errno != 9) return 4;\n"
        "  c = 7;\n"
        "  if (sendto(sv[0], &c, 1, 0, 0, 0) != 1 || errno != 0) return 5;\n"
        "  close(sv[0]);\n"
        "  close(sv[1]);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("sendto_user",
        "package main;\n"
        "long sendto(long fd, char *buf, long n, long flags, char *addr, long alen) { return n ? 3 : 3; }\n"
        "int main(void) { return sendto(0, 0, 1, 0, 0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("getpeername_unix",
        "package main;\n"
        "int errno;\n"
        "long socket(long domain, long type, long proto);\n"
        "long bind(long fd, char *addr, long len);\n"
        "long listen(long fd, long backlog);\n"
        "long connect(long fd, char *addr, long len);\n"
        "long getpeername(long fd, char *addr, int *len);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_peer_rt\");\n"
        "  long srv = socket(1, 1, 0);\n"
        "  if (srv < 0) return 1;\n"
        "  char *s = \"/tmp/fakecc_peer_rt\";\n"
        "  int n = 0;\n"
        "  while (s[n]) n++;\n"
        "  char addr[108];\n"
        "  int i;\n"
        "  for (i = 0; i < 108; i++) addr[i] = 0;\n"
        "  addr[0] = (char)(2 + n);\n"
        "  addr[1] = 1;\n"
        "  for (i = 0; i < n; i++) addr[2 + i] = s[i];\n"
        "  if (bind(srv, addr, 2 + n) != 0) return 2;\n"
        "  if (listen(srv, 1) != 0) return 3;\n"
        "  long cli = socket(1, 1, 0);\n"
        "  if (cli < 0) return 4;\n"
        "  char peer[108];\n"
        "  int plen = 108;\n"
        "  if (getpeername(cli, peer, &plen) != -57 || errno != 57) return 5;\n"
        "  if (connect(cli, addr, 2 + n) != 0 || errno != 0) return 6;\n"
        "  plen = 108;\n"
        "  if (getpeername(cli, peer, &plen) != 0) return 8;\n"
        "  if (peer[1] != 1 || plen < 2 + n) return 9;\n"
        "  for (i = 0; i < n; i++) if (peer[2 + i] != s[i]) return 10;\n"
        "  if (getpeername(-1, peer, &plen) != -9 || errno != 9) return 11;\n"
        "  close(cli);\n"
        "  close(srv);\n"
        "  unlink(\"/tmp/fakecc_peer_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getpeername_user",
        "package main;\n"
        "long getpeername(long fd, char *addr, int *len) { return fd ? 3 : 3; }\n"
        "int main(void) { return getpeername(1, 0, 0) == 3 ? 7 : 1; }\n", 7);
    expect("poll_socket",
        "package main;\n"
        "int errno;\n"
        "long socketpair(long domain, long type, long proto, int *sv);\n"
        "long poll(int *fds, long nfds, long timeout);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  int sv[2];\n"
        "  if (socketpair(1, 1, 0, sv) != 0) return 1;\n"
        "  int p[2];\n"
        "  p[0] = sv[0];\n"
        "  p[1] = 4;\n"
        "  if (poll(p, -1, 0) != -22 || errno != 22) return 2;\n"
        "  if (poll(p, 1, 0) != 1 || errno != 0) return 3;\n"
        "  int rev = (p[1] >> 16) & 0xffff;\n"
        "  if ((rev & 4) == 0) return 4;\n"
        "  int bad[2];\n"
        "  bad[0] = 200;\n"
        "  bad[1] = 1;\n"
        "  if (poll(bad, 1, 0) != 1) return 5;\n"
        "  int nval = (bad[1] >> 16) & 0xffff;\n"
        "  if ((nval & 32) == 0) return 6;\n"
        "  if (poll(0, 0, 0) != 0) return 8;\n"
        "  close(sv[0]);\n"
        "  close(sv[1]);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("poll_user",
        "package main;\n"
        "long poll(int *fds, long nfds, long timeout) { return nfds ? 3 : 3; }\n"
        "int main(void) { return poll(0, 1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("kqueue_empty",
        "package main;\n"
        "int errno;\n"
        "long kqueue(void);\n"
        "long kevent(long kq, long *chg, long nchg, long *ev, long nev, long *ts);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  long fd = kqueue();\n"
        "  if (fd < 0) return 1;\n"
        "  long ts[2];\n"
        "  ts[0] = 0; ts[1] = 0;\n"
        "  if (kevent(fd, 0, 0, 0, 0, ts) != 0) return 2;\n"
        "  if (kevent(-1, 0, 0, 0, 0, ts) != -9 || errno != 9) return 3;\n"
        "  if (kevent(fd, 0, 0, 0, 0, ts) != 0 || errno != 0) return 4;\n"
        "  close(fd);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("kqueue_user",
        "package main;\n"
        "long kqueue(void) { return 3; }\n"
        "int main(void) { return kqueue() == 3 ? 7 : 1; }\n", 7);
    expect("fchmod_mode",
        "package main;\n"
        "int errno;\n"
        "long fchmod(long fd, long mode);\n"
        "long access(char *path, long mode);\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long unlink(char *p);\n"
        "int main(void) {\n"
        "  unlink(\"/tmp/fakecc_fchmod_rt\");\n"
        "  long fd = open(\"/tmp/fakecc_fchmod_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 1;\n"
        "  if (fchmod(fd, 0) != 0) return 2;\n"
        "  if (access(\"/tmp/fakecc_fchmod_rt\", 4) != -13 || errno != 13) return 3;\n"
        "  if (fchmod(fd, 420) != 0 || errno != 0) return 4;\n"
        "  if (access(\"/tmp/fakecc_fchmod_rt\", 4) != 0) return 5;\n"
        "  if (fchmod(-1, 420) != -9 || errno != 9) return 6;\n"
        "  close(fd);\n"
        "  unlink(\"/tmp/fakecc_fchmod_rt\");\n"
        "  return 7;\n"
        "}\n", 7);
    expect("fchmod_user",
        "package main;\n"
        "long fchmod(long fd, long mode) { return mode ? 3 : 3; }\n"
        "int main(void) { return fchmod(1, 420) == 3 ? 7 : 1; }\n", 7);
    expect("madvise_page",
        "package main;\n"
        "int errno;\n"
        "long madvise(long addr, long len, long advice);\n"
        "long mincore(long addr, long len, char *vec);\n"
        "int main(void) {\n"
        "  long p = __syscall(197, 0, 16384, 3, 0x1002, -1, 0);\n"
        "  if (p < 0) return 1;\n"
        "  if (madvise(p + 1, 16384, 0) != -22 || errno != 22) return 2;\n"
        "  if (madvise(p, 16384, 2) != 0 || errno != 0) return 3;\n"
        "  char *q = (char *)p;\n"
        "  q[0] = 1;\n"
        "  char v = 0;\n"
        "  if (mincore(p, 16384, &v) != 0) return 4;\n"
        "  if ((v & 1) == 0) return 5;\n"
        "  if (__syscall(73, p, 16384) != 0) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("madvise_user",
        "package main;\n"
        "long madvise(long addr, long len, long advice) { return advice ? 3 : 3; }\n"
        "int main(void) { return madvise(0, 0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("mprotect_page",
        "package main;\n"
        "int errno;\n"
        "long mprotect(long addr, long len, long prot);\n"
        "long msync(long addr, long len, long flags);\n"
        "int main(void) {\n"
        "  long p = __syscall(197, 0, 16384, 3, 0x1002, -1, 0);\n"
        "  if (p < 0) return 1;\n"
        "  if (mprotect(p, 16384, 1) != 0) return 2;\n"
        "  if (mprotect(p, 16384, 3) != 0) return 3;\n"
        "  char *q = (char *)p;\n"
        "  q[0] = 7;\n"
        "  if (q[0] != 7) return 4;\n"
        "  if (mprotect(p + 1, 16384, 3) != -22 || errno != 22) return 5;\n"
        "  if (msync(p, 16384, 1) != 0 || errno != 0) return 6;\n"
        "  if (__syscall(73, p, 16384) != 0) return 8;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("mprotect_user",
        "package main;\n"
        "long mprotect(long addr, long len, long prot) { return prot ? 3 : 3; }\n"
        "int main(void) { return mprotect(0, 0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("mlock_page",
        "package main;\n"
        "int errno;\n"
        "long mlock(long addr, long len);\n"
        "long munlock(long addr, long len);\n"
        "int main(void) {\n"
        "  long p = __syscall(197, 0, 16384, 3, 0x1002, -1, 0);\n"
        "  if (p < 0) return 1;\n"
        "  char *q = (char *)p;\n"
        "  q[0] = 1;\n"
        "  if (mlock(p, 16384) != 0) return 2;\n"
        "  if (munlock(p, 16384) != 0) return 3;\n"
        "  if (mlock(p + 1, 16384) != -12 || errno != 12) return 4;\n"
        "  if (mlock(p, 16384) != 0 || errno != 0) return 5;\n"
        "  munlock(p, 16384);\n"
        "  if (__syscall(73, p, 16384) != 0) return 6;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("mlock_user",
        "package main;\n"
        "long mlock(long addr, long len) { return len ? 3 : 3; }\n"
        "int main(void) { return mlock(0, 1) == 3 ? 7 : 1; }\n", 7);
    expect("priority_self",
        "package main;\n"
        "int errno;\n"
        "long getpriority(long which, long who);\n"
        "long setpriority(long which, long who, long prio);\n"
        "int main(void) {\n"
        "  long cur = getpriority(0, 0);\n"
        "  if (cur < 0 || cur > 19) return 1;\n"
        "  if (setpriority(0, 0, cur + 1) != 0) return 2;\n"
        "  if (getpriority(0, 0) != cur + 1) return 3;\n"
        "  if (getpriority(99, 0) != -22 || errno != 22) return 4;\n"
        "  if (getpriority(0, 0) != cur + 1 || errno != 0) return 5;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("priority_user",
        "package main;\n"
        "long getpriority(long which, long who) { return which ? 3 : 3; }\n"
        "int main(void) { return getpriority(1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("rusage_self",
        "package main;\n"
        "int errno;\n"
        "long getrusage(long who, long *ru);\n"
        "int main(void) {\n"
        "  long ru[32];\n"
        "  int i;\n"
        "  for (i = 0; i < 32; i++) ru[i] = -1;\n"
        "  if (getrusage(99, ru) != -22 || errno != 22) return 1;\n"
        "  if (getrusage(0, ru) != 0 || errno != 0) return 2;\n"
        "  if (ru[0] < 0) return 3;\n"
        "  if (ru[4] <= 0) return 4;\n"
        "  if (getrusage(-1, ru) != 0) return 5;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("rusage_user",
        "package main;\n"
        "long getrusage(long who, long *ru) { return who ? 3 : 3; }\n"
        "int main(void) { return getrusage(1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("openat_tmp",
        "package main;\n"
        "int errno;\n"
        "long open(char *p, long flags, long mode);\n"
        "long openat(long dirfd, char *path, long flags, long mode);\n"
        "long unlinkat(long dirfd, char *path, long flags);\n"
        "long close(long fd);\n"
        "int main(void) {\n"
        "  long dir = open(\"/tmp\", 0, 0);\n"
        "  if (dir < 0) return 1;\n"
        "  unlinkat(dir, \"fakecc_ulat_rt\", 0);\n"
        "  long fd = openat(dir, \"fakecc_ulat_rt\", 0x602, 420);\n"
        "  if (fd < 0) return 2;\n"
        "  close(fd);\n"
        "  if (unlinkat(dir, \"fakecc_ulat_rt\", 0) != 0) return 3;\n"
        "  if (unlinkat(dir, \"fakecc_ulat_rt\", 0) != -2 || errno != 2) return 4;\n"
        "  if (openat(200, \"fakecc_ulat_rt\", 0, 0) != -9 || errno != 9) return 5;\n"
        "  fd = openat(dir, \"fakecc_ulat_rt\", 0x602, 420);\n"
        "  if (fd < 0 || errno != 0) return 6;\n"
        "  close(fd);\n"
        "  unlinkat(dir, \"fakecc_ulat_rt\", 0);\n"
        "  close(dir);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("openat_user",
        "package main;\n"
        "long openat(long dirfd, char *path, long flags, long mode) { return flags ? 3 : 3; }\n"
        "int main(void) { return openat(0, 0, 1, 0) == 3 ? 7 : 1; }\n", 7);
    expect("mkdirat_tmp",
        "package main;\n"
        "int errno;\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long mkdirat(long dirfd, char *path, long mode);\n"
        "long renameat(long olddir, char *old, long newdir, char *newp);\n"
        "long rmdir(char *p);\n"
        "long access(char *p, long mode);\n"
        "int main(void) {\n"
        "  rmdir(\"/tmp/fakecc_mkdirat_rt\");\n"
        "  rmdir(\"/tmp/fakecc_mkdirat_rt2\");\n"
        "  long dir = open(\"/tmp\", 0, 0);\n"
        "  if (dir < 0) return 1;\n"
        "  if (mkdirat(dir, \"fakecc_mkdirat_rt\", 448) != 0) return 2;\n"
        "  if (mkdirat(dir, \"fakecc_mkdirat_rt\", 448) != -17 || errno != 17) return 3;\n"
        "  if (renameat(dir, \"fakecc_mkdirat_rt\", dir, \"fakecc_mkdirat_rt2\") != 0 || errno != 0) return 4;\n"
        "  if (access(\"/tmp/fakecc_mkdirat_rt\", 0) != -2) return 5;\n"
        "  if (rmdir(\"/tmp/fakecc_mkdirat_rt2\") != 0) return 6;\n"
        "  if (mkdirat(200, \"fakecc_mkdirat_rt\", 448) != -9 || errno != 9) return 8;\n"
        "  close(dir);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("mkdirat_user",
        "package main;\n"
        "long mkdirat(long dirfd, char *path, long mode) { return mode ? 3 : 3; }\n"
        "int main(void) { return mkdirat(0, 0, 448) == 3 ? 7 : 1; }\n", 7);
    expect("faccessat_tmp",
        "package main;\n"
        "int errno;\n"
        "long open(char *p, long flags, long mode);\n"
        "long close(long fd);\n"
        "long faccessat(long dirfd, char *path, long mode, long flags);\n"
        "int main(void) {\n"
        "  long dir = open(\"/tmp\", 0, 0);\n"
        "  if (dir < 0) return 1;\n"
        "  if (faccessat(dir, \".\", 0, 0) != 0) return 2;\n"
        "  if (faccessat(dir, \"no_such_fakecc_fa\", 0, 0) != -2 || errno != 2) return 3;\n"
        "  if (faccessat(200, \"no_such_fakecc_fa\", 0, 0) != -9 || errno != 9) return 4;\n"
        "  if (faccessat(dir, \".\", 0, 0) != 0 || errno != 0) return 5;\n"
        "  close(dir);\n"
        "  return 7;\n"
        "}\n", 7);
    expect("faccessat_user",
        "package main;\n"
        "long faccessat(long dirfd, char *path, long mode, long flags) { return flags ? 3 : 3; }\n"
        "int main(void) { return faccessat(0, 0, 0, 1) == 3 ? 7 : 1; }\n", 7);
}

static void test_getenv(void) {
    expect("getenv_path",
        "package main;\n"
        "char *getenv(char *name);\n"
        "int main(void) {\n"
        "  char *p = getenv(\"PATH\");\n"
        "  if (!p || !p[0]) return 1;\n"
        "  if (p[0]=='P' && p[1]=='A' && p[2]=='T' && p[3]=='H' && p[4]=='=')\n"
        "    return 2;\n"
        "  if (getenv(\"PAT\")) return 3;\n"
        "  if (getenv(\"FAKECC_NO_SUCH_VAR_ZZ\")) return 4;\n"
        "  return 7;\n"
        "}\n", 7);
    expect("getenv_user",
        "package main;\n"
        "char *getenv(char *name) { return name ? 0 : 0; }\n"
        "int main(void) { return getenv(\"PATH\") ? 1 : 7; }\n", 7);
}

static void test_macho_link(void) {
    expect("label_addr",
        "package main;\n"
        "int main(void) {\n"
        "  void *p = &&done;\n"
        "  goto *p;\n"
        "  return 1;\n"
        "done:\n"
        "  return 7;\n"
        "}", 7);
    const char *la = "/tmp/fakecc_arm64_link_la.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int main(void) {\n"
        "  void *p = &&done;\n"
        "  goto *p;\n"
        "  return 1;\n"
        "done:\n"
        "  return 7;\n"
        "}\n",
        la, NULL), 0);
    EmitModule mla;
    T_ASSERT_EQ_INT(emit_obj_read(la, &mla), 0);
    EmitModule *lam[1] = { &mla };
    T_ASSERT_EQ_INT(macho_link_objects(lam, 1, "/tmp/fakecc_arm64_link_out"), 0);
    T_ASSERT_EQ_INT(macho_codesign("/tmp/fakecc_arm64_link_out"), 0);
    T_ASSERT_EQ_INT(run_bin("/tmp/fakecc_arm64_link_out"), 7);
    emit_module_free(&mla);

    const char *a = "/tmp/fakecc_arm64_link_a.o";
    const char *b = "/tmp/fakecc_arm64_link_b.o";
    const char *outp = "/tmp/fakecc_arm64_link_out";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add(int a, int b);\n"
        "int main(void) { return add(20, 22); }\n",
        a, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add(int a, int b) { return a + b; }\n",
        b, NULL), 0);
    EmitModule ma, mb;
    T_ASSERT_EQ_INT(emit_obj_read(a, &ma), 0);
    T_ASSERT_EQ_INT(emit_obj_read(b, &mb), 0);
    EmitModule *mods[2] = { &ma, &mb };
    T_ASSERT_EQ_INT(macho_link_objects(mods, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 42);
    emit_module_free(&ma);
    emit_module_free(&mb);

    const char *g = "/tmp/fakecc_arm64_link_g.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int g = 7;\n"
        "int main(void) { return g; }\n",
        g, NULL), 0);
    EmitModule mg;
    T_ASSERT_EQ_INT(emit_obj_read(g, &mg), 0);
    EmitModule *one[1] = { &mg };
    T_ASSERT_EQ_INT(macho_link_objects(one, 1, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&mg);

    const char *pa = "/tmp/fakecc_arm64_link_pa.o";
    const char *pb = "/tmp/fakecc_arm64_link_pb.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int g;\n"
        "int *p = &g;\n"
        "int main(void) { return *p; }\n",
        pa, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int g = 7;\n",
        pb, NULL), 0);
    EmitModule mpa, mpb;
    T_ASSERT_EQ_INT(emit_obj_read(pa, &mpa), 0);
    T_ASSERT_EQ_INT(emit_obj_read(pb, &mpb), 0);
    EmitModule *pm[2] = { &mpa, &mpb };
    T_ASSERT_EQ_INT(macho_link_objects(pm, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&mpa);
    emit_module_free(&mpb);

    const char *ta = "/tmp/fakecc_arm64_link_ta.o";
    const char *tb = "/tmp/fakecc_arm64_link_tb.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int g;\n"
        "int main(void) { return g; }\n",
        ta, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int g = 7;\n",
        tb, NULL), 0);
    EmitModule mta, mtb;
    T_ASSERT_EQ_INT(emit_obj_read(ta, &mta), 0);
    T_ASSERT_EQ_INT(emit_obj_read(tb, &mtb), 0);
    int gi = emit_module_find_symbol(&mta, "g");
    T_ASSERT(gi >= 0);
    T_ASSERT_EQ_INT(mta.syms[gi].shndx, SHN_COMMON);
    EmitModule *tm[2] = { &mta, &mtb };
    T_ASSERT_EQ_INT(macho_link_objects(tm, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&mta);
    emit_module_free(&mtb);

    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int g;\n"
        "int main(void) { g = 4; return g; }\n",
        ta, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int g;\n",
        tb, NULL), 0);
    T_ASSERT_EQ_INT(emit_obj_read(ta, &mta), 0);
    T_ASSERT_EQ_INT(emit_obj_read(tb, &mtb), 0);
    EmitModule *cm[2] = { &mta, &mtb };
    T_ASSERT_EQ_INT(macho_link_objects(cm, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 4);
    emit_module_free(&mta);
    emit_module_free(&mtb);

    const char *ca = "/tmp/fakecc_arm64_link_ca.o";
    const char *cb = "/tmp/fakecc_arm64_link_cb.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "__attribute__((constructor(200))) void late(void){ __syscall(1,21); }\n"
        "int main(void) { return 0; }\n",
        ca, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "__attribute__((constructor(100))) void early(void){ __syscall(1,11); }\n",
        cb, NULL), 0);
    EmitModule mca, mcb;
    T_ASSERT_EQ_INT(emit_obj_read(ca, &mca), 0);
    T_ASSERT_EQ_INT(emit_obj_read(cb, &mcb), 0);
    T_ASSERT(mca.init_array.len == 8);
    T_ASSERT_EQ_INT(mca.init_prio[0], 200);
    T_ASSERT_EQ_INT(mcb.init_prio[0], 100);
    EmitModule *cm2[2] = { &mca, &mcb };
    T_ASSERT_EQ_INT(macho_link_objects(cm2, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 11);
    emit_module_free(&mca);
    emit_module_free(&mcb);

    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "__attribute__((destructor(100))) void a(void){ __syscall(1,31); }\n"
        "int main(void) { return 9; }\n",
        ca, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "__attribute__((destructor(200))) void b(void){ __syscall(1,32); }\n",
        cb, NULL), 0);
    T_ASSERT_EQ_INT(emit_obj_read(ca, &mca), 0);
    T_ASSERT_EQ_INT(emit_obj_read(cb, &mcb), 0);
    EmitModule *dm[2] = { &mca, &mcb };
    T_ASSERT_EQ_INT(macho_link_objects(dm, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 32);
    emit_module_free(&mca);
    emit_module_free(&mcb);

    const char *ea = "/tmp/fakecc_arm64_link_ea.o";
    const char *eb = "/tmp/fakecc_arm64_link_eb.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern const int c;\n"
        "extern int a[4];\n"
        "int *p = &a[1];\n"
        "int main(void) { return c + *p; }\n",
        ea, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "const int c = 3;\n"
        "int a[4] = {1, 2, 3, 4};\n",
        eb, NULL), 0);
    EmitModule mea, meb;
    T_ASSERT_EQ_INT(emit_obj_read(ea, &mea), 0);
    T_ASSERT_EQ_INT(emit_obj_read(eb, &meb), 0);
    int cdef = emit_module_find_symbol(&meb, "c");
    int adef = emit_module_find_symbol(&meb, "a");
    T_ASSERT(cdef >= 0);
    T_ASSERT(adef >= 0);
    T_ASSERT_EQ_INT(meb.syms[cdef].shndx, SECT_RODATA);
    T_ASSERT_EQ_INT(meb.syms[adef].shndx, SECT_DATA);
    int cu = emit_module_find_symbol(&mea, "c");
    T_ASSERT(cu >= 0);
    T_ASSERT_EQ_INT(mea.syms[cu].shndx, 0);
    EmitModule *em[2] = { &mea, &meb };
    T_ASSERT_EQ_INT(macho_link_objects(em, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 5);
    emit_module_free(&mea);
    emit_module_free(&meb);

    const char *fa = "/tmp/fakecc_arm64_link_fa.o";
    const char *fb = "/tmp/fakecc_arm64_link_fb.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add(int a, int b) { return a + b; }\n"
        "int main(void) {\n"
        "  int (*p)(int, int) = add;\n"
        "  return p(20, 22);\n"
        "}\n",
        fa, NULL), 0);
    EmitModule mfa;
    T_ASSERT_EQ_INT(emit_obj_read(fa, &mfa), 0);
    EmitModule *onef[1] = { &mfa };
    T_ASSERT_EQ_INT(macho_link_objects(onef, 1, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 42);
    emit_module_free(&mfa);

    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add(int a, int b);\n"
        "int main(void) {\n"
        "  int (*p)(int, int) = add;\n"
        "  return p(20, 22);\n"
        "}\n",
        fa, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int add(int a, int b) { return a + b; }\n",
        fb, NULL), 0);
    EmitModule mfb;
    T_ASSERT_EQ_INT(emit_obj_read(fa, &mfa), 0);
    T_ASSERT_EQ_INT(emit_obj_read(fb, &mfb), 0);
    int addu = emit_module_find_symbol(&mfa, "add");
    T_ASSERT(addu >= 0);
    T_ASSERT_EQ_INT(mfa.syms[addu].shndx, 0);
    EmitModule *fm[2] = { &mfa, &mfb };
    T_ASSERT_EQ_INT(macho_link_objects(fm, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 42);
    emit_module_free(&mfa);
    emit_module_free(&mfb);

    const char *aa = "/tmp/fakecc_arm64_link_aa.o";
    const char *ab = "/tmp/fakecc_arm64_link_ab.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "int small = 1;\n"
        "const int sc = 3;\n",
        aa, NULL), 0);
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "extern int small;\n"
        "extern const int sc;\n"
        "int g __attribute__((aligned(64))) = 1;\n"
        "const int c __attribute__((aligned(64))) = 2;\n"
        "static int zb __attribute__((aligned(64)));\n"
        "int main(void) {\n"
        "  if (((unsigned long)&g) & 63) return 1;\n"
        "  if (((unsigned long)&c) & 63) return 2;\n"
        "  if (((unsigned long)&zb) & 63) return 3;\n"
        "  zb = 4;\n"
        "  if (zb != 4 || small != 1 || sc != 3) return 5;\n"
        "  return 7;\n"
        "}\n",
        ab, NULL), 0);
    EmitModule maa, mab;
    T_ASSERT_EQ_INT(emit_obj_read(aa, &maa), 0);
    T_ASSERT_EQ_INT(emit_obj_read(ab, &mab), 0);
    EmitModule *am[2] = { &maa, &mab };
    T_ASSERT_EQ_INT(macho_link_objects(am, 2, outp), 0);
    T_ASSERT_EQ_INT(macho_codesign(outp), 0);
    T_ASSERT_EQ_INT(run_bin(outp), 7);
    emit_module_free(&maa);
    emit_module_free(&mab);

    const char *pc = "/tmp/fakecc_arm64_link_pc.o";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(
        "package main;\n"
        "static int g = 1;\n"
        "static int *const cp = &g;\n"
        "static const int *pp = &g;\n"
        "static const int k = 4;\n"
        "int main(void) { return *cp + *pp + k; }\n",
        pc, NULL), 0);
    EmitModule mpc;
    T_ASSERT_EQ_INT(emit_obj_read(pc, &mpc), 0);
    int ksym = emit_module_find_symbol(&mpc, "k");
    int cpsym = emit_module_find_symbol(&mpc, "cp");
    int ppsym = emit_module_find_symbol(&mpc, "pp");
    T_ASSERT(ksym >= 0 && cpsym >= 0 && ppsym >= 0);
    T_ASSERT_EQ_INT(mpc.syms[ksym].shndx, SECT_RODATA);
    T_ASSERT_EQ_INT(mpc.syms[cpsym].shndx, SECT_DATA);
    T_ASSERT_EQ_INT(mpc.syms[ppsym].shndx, SECT_DATA);
    emit_module_free(&mpc);
}

static void test_frame_addr(void) {
    expect("frame",
        "package main;\n"
        "__attribute__((noinline)) int child(void *parent) {\n"
        "  void *f0 = __builtin_frame_address(0);\n"
        "  void *f1 = __builtin_frame_address(1);\n"
        "  if (f0 == 0) return 1;\n"
        "  if (f1 != parent) return 2;\n"
        "  if (__builtin_return_address(0) == 0) return 3;\n"
        "  if (__builtin_return_address(1) == 0) return 4;\n"
        "  return 0;\n"
        "}\n"
        "int main(void) {\n"
        "  if (__builtin_expect(1, 0) != 1) return 5;\n"
        "  int n = 8;\n"
        "  char *p = __builtin_alloca(n);\n"
        "  p[0] = 7; p[7] = 9;\n"
        "  if (p[0] != 7 || p[7] != 9) return 6;\n"
        "  return child(__builtin_frame_address(0));\n"
        "}", 0);
    expect("ovf",
        "package main;\n"
        "int main(void) {\n"
        "  int r = 0;\n"
        "  if (!__builtin_add_overflow(2000000000, 2000000000, &r)) return 1;\n"
        "  if (__builtin_add_overflow(2, 3, &r)) return 2;\n"
        "  if (r != 5) return 3;\n"
        "  unsigned u = 0;\n"
        "  if (!__builtin_add_overflow(0xffffffffu, 1u, &u)) return 4;\n"
        "  if (u != 0) return 5;\n"
        "  return 0; }", 0);
}

static void test_syscall(void) {
    /* Carry set + positive errno becomes -errno.  EBADF is 9.
     * Anonymous mmap is Darwin mmap=197, PROT_READ|PROT_WRITE=3,
     * MAP_PRIVATE|MAP_ANON=0x1002. */
    expect("sys_err",
        "package main;\n"
        "int main(void) {\n"
        "  long r = __syscall(4, -1, 0, 0);\n"
        "  if (r != -9) return 1;\n"
        "  long z = __syscall(4, 1, 0, 0);\n"
        "  if (z != 0) return 2;\n"
        "  long p = __syscall(20);\n"
        "  if (p <= 0) return 3;\n"
        "  long m = __syscall(197, 0, 16384, 3, 0x1002, -1, 0);\n"
        "  if (m <= 0) return 4;\n"
        "  char *page = (char *)m;\n"
        "  page[0] = 42;\n"
        "  if (page[0] != 42) return 5;\n"
        "  return 0; }", 0);
}

static void test_setjmp(void) {
    expect("sj_same",
        "package main;\n"
        "int main(void) {\n"
        "  void *buf[5];\n"
        "  int n = 0;\n"
        "  if (__builtin_setjmp(buf) == 0) {\n"
        "    n = 1;\n"
        "    __builtin_longjmp(buf, 1);\n"
        "    return 1;\n"
        "  }\n"
        "  return n == 1 ? 7 : 2; }", 7);
    expect("sj_hop",
        "package main;\n"
        "void *buf[5];\n"
        "void hop(void) { __builtin_longjmp(buf, 1); }\n"
        "int main(void) {\n"
        "  int x = 20;\n"
        "  if (__builtin_setjmp(buf) == 0) {\n"
        "    x = 22;\n"
        "    hop();\n"
        "    return 1;\n"
        "  }\n"
        "  return x == 22 ? 7 : 2; }", 7);
    expect("sj_alloca",
        "package main;\n"
        "void *buf[5];\n"
        "void hop(void) { __builtin_longjmp(buf, 1); }\n"
        "int main(void) {\n"
        "  char *p = __builtin_alloca(4);\n"
        "  p[0] = 7;\n"
        "  if (__builtin_setjmp(buf)) return p[0] == 7 ? 7 : 3;\n"
        "  char *q = __builtin_alloca(32);\n"
        "  q[0] = 1;\n"
        "  hop();\n"
        "  return 1; }", 7);
}

static void test_bitops(void) {
    expect("bits",
        "package main;\n"
        "int main(void) {\n"
        "  unsigned a = 1;\n"
        "  unsigned long long b = 1ull << 40;\n"
        "  if (__builtin_clz(a) != 31) return 1;\n"
        "  if (__builtin_clzll(b) != 23) return 2;\n"
        "  if (__builtin_ctz(a << 3) != 3) return 3;\n"
        "  if (__builtin_ctzll(b) != 40) return 4;\n"
        "  if (__builtin_ffs(0) != 0) return 5;\n"
        "  if (__builtin_ffs((int)(a << 3)) != 4) return 6;\n"
        "  if (__builtin_ffsll(0) != 0) return 7;\n"
        "  if (__builtin_ffsll((long long)b) != 41) return 8;\n"
        "  if (__builtin_popcount(0xF0u + 0x0Fu) != 8) return 9;\n"
        "  if (__builtin_popcountll(b - 1) != 40) return 10;\n"
        "  if (__builtin_parity(7u) != 1) return 11;\n"
        "  if (__builtin_parityll(0xFull) != 0) return 12;\n"
        "  if (__builtin_bswap16(0x1234) != 0x3412) return 13;\n"
        "  if (__builtin_bswap32(0x12345678u) != 0x78563412u) return 14;\n"
        "  unsigned long long s = __builtin_bswap64(0x0102030405060708ull);\n"
        "  if ((int)(s & 0xff) != 1) return 15;\n"
        "  if ((int)(s >> 56) != 8) return 20;\n"
        "  if (__builtin_clrsb(1) != 30) return 16;\n"
        "  if (__builtin_clrsb(-1) != 31) return 17;\n"
        "  if (__builtin_clrsbl(1L) != 62) return 18;\n"
        "  if (__builtin_clzl(1L) != 63) return 19;\n"
        "  return 0; }", 0);
}

static void test_fpmath(void) {
    expect("fpmath",
        "package main;\n"
        "int main(void) {\n"
        "  double a = -3.5;\n"
        "  float b = -2.5f;\n"
        "  if (__builtin_fabs(a) != 3.5) return 1;\n"
        "  if (__builtin_fabsf(b) != 2.5f) return 2;\n"
        "  if (__builtin_fabsl(-8.0L) != 8.0L) return 3;\n"
        "  if (__builtin_sqrt(9.0) != 3.0) return 4;\n"
        "  if (__builtin_sqrtf(4.0f) != 2.0f) return 5;\n"
        "  if (__builtin_ceil(1.2) != 2.0) return 6;\n"
        "  if (__builtin_floor(-1.2) != -2.0) return 7;\n"
        "  if (__builtin_trunc(-1.8) != -1.0) return 8;\n"
        "  if (__builtin_round(2.5) != 3.0) return 9;\n"
        "  if (__builtin_nearbyint(2.5) != 2.0) return 10;\n"
        "  if (__builtin_fmin(1.5, -4.0) != -4.0) return 11;\n"
        "  if (__builtin_fmaxf(-1.0f, 4.0f) != 4.0f) return 12;\n"
        "  if (__builtin_fma(2.0, 3.0, 4.0) != 10.0) return 13;\n"
        "  if (__builtin_fmaf(-2.0f, 3.0f, 1.0f) != -5.0f) return 14;\n"
        "  if (__builtin_copysign(2.0, -1.0) != -2.0) return 15;\n"
        "  if (__builtin_copysign(-2.0, 1.0) != 2.0) return 16;\n"
        "  if (__builtin_copysignf(-3.0f, 1.0f) != 3.0f) return 17;\n"
        "  if (__builtin_fmin(__builtin_nan(\"\"), 3.0) != 3.0) return 18;\n"
        "  return 0; }", 0);
}

static void test_scan(void) {
    expect("scan",
        "package main;\n"
        "int main(void) {\n"
        "  char a[] = {'h','i',0};\n"
        "  char b[] = {'h','i',0};\n"
        "  char c[] = {'h','j',0};\n"
        "  char d[] = {'h','i','!',0};\n"
        "  if (__builtin_strlen(a) != 2) return 1;\n"
        "  if (__builtin_strlen(\"\") != 0) return 2;\n"
        "  if (__builtin_memcmp(a, b, 3) != 0) return 3;\n"
        "  if (__builtin_memcmp(a, c, 2) >= 0) return 4;\n"
        "  if (__builtin_memcmp(a, b, 0) != 0) return 5;\n"
        "  if (__builtin_strcmp(a, b) != 0) return 6;\n"
        "  if (__builtin_strcmp(a, c) >= 0) return 7;\n"
        "  if (__builtin_strcmp(d, a) <= 0) return 8;\n"
        "  if (__builtin_strncmp(a, c, 1) != 0) return 9;\n"
        "  if (__builtin_strncmp(a, c, 2) >= 0) return 10;\n"
        "  return 0; }", 0);
}

static void test_find(void) {
    expect("find",
        "package main;\n"
        "int main(void) {\n"
        "  char a[] = {1,2,3,2,0};\n"
        "  char *p = __builtin_memchr(a, 3, 4);\n"
        "  if (!p || *p != 3) return 1;\n"
        "  if (__builtin_memchr(a, 9, 4)) return 2;\n"
        "  if (__builtin_memchr(a, 1, 0)) return 3;\n"
        "  p = __builtin_strchr(a, 2);\n"
        "  if (p != a + 1) return 4;\n"
        "  if (__builtin_strchr(a, 9)) return 5;\n"
        "  if (__builtin_strchr(a, 0) != a + 4) return 6;\n"
        "  p = __builtin_strrchr(a, 2);\n"
        "  if (p != a + 3) return 7;\n"
        "  if (__builtin_strrchr(a, 0) != a + 4) return 8;\n"
        "  if (__builtin_strnlen(a, 100) != 4) return 9;\n"
        "  if (__builtin_strnlen(a, 2) != 2) return 10;\n"
        "  if (__builtin_strnlen(a, 0) != 0) return 11;\n"
        "  return 0; }", 0);
}

static void test_pad(void) {
    expect("pad",
        "package main;\n"
        "int main(void) {\n"
        "  char d[4]; d[0] = 1; d[1] = 2; d[2] = 3; d[3] = 4;\n"
        "  __builtin_bzero(d, 3);\n"
        "  if (d[0] != 0 || d[1] != 0 || d[2] != 0 || d[3] != 4) return 1;\n"
        "  char s[3]; s[0] = 5; s[1] = 6; s[2] = 7;\n"
        "  char *p = (char *)__builtin_mempcpy(d, s, 3);\n"
        "  if (p != d + 3 || d[0] != 5 || d[2] != 7) return 2;\n"
        "  char e[4];\n"
        "  p = __builtin_stpncpy(e, \"ab\", 4);\n"
        "  if (p != e + 2 || e[2] != 0 || e[3] != 0) return 3;\n"
        "  e[3] = 9;\n"
        "  p = __builtin_stpncpy(e, \"abcd\", 3);\n"
        "  if (p != e + 3 || e[0] != 97 || e[2] != 99 || e[3] != 9) return 4;\n"
        "  p = __builtin_stpncpy(e, \"\", 0);\n"
        "  if (p != e) return 5;\n"
        "  return 0; }", 0);
}

static void test_span(void) {
    expect("span",
        "package main;\n"
        "int main(void) {\n"
        "  if (__builtin_strspn(\"hello\", \"he\") != 2) return 1;\n"
        "  if (__builtin_strspn(\"hello\", \"xyz\") != 0) return 2;\n"
        "  if (__builtin_strspn(\"hello\", \"\") != 0) return 3;\n"
        "  if (__builtin_strspn(\"\", \"abc\") != 0) return 4;\n"
        "  if (__builtin_strspn(\"aaab\", \"a\") != 3) return 5;\n"
        "  if (__builtin_strcspn(\"hello\", \"l\") != 2) return 6;\n"
        "  if (__builtin_strcspn(\"hello\", \"xyz\") != 5) return 7;\n"
        "  if (__builtin_strcspn(\"hello\", \"\") != 5) return 8;\n"
        "  if (__builtin_strcspn(\"\", \"abc\") != 0) return 9;\n"
        "  if (__builtin_strcspn(\"hello\", \"h\") != 0) return 10;\n"
        "  return 0; }", 0);
}

static void test_atomic(void) {
    expect("atomic",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 1;\n"
        "  if (__atomic_load_n(&x, 0) != 1) return 1;\n"
        "  __atomic_store_n(&x, 9, 0);\n"
        "  if (x != 9) return 2;\n"
        "  if (__atomic_exchange_n(&x, 4, 0) != 9 || x != 4) return 3;\n"
        "  if (__atomic_fetch_add(&x, 3, 0) != 4 || x != 7) return 4;\n"
        "  if (__atomic_add_fetch(&x, 1, 0) != 8) return 5;\n"
        "  if (__atomic_fetch_and(&x, 15, 0) != 8 || x != 8) return 6;\n"
        "  int exp = 8;\n"
        "  if (!__atomic_compare_exchange_n(&x, &exp, 2, 0, 0, 0) || x != 2) return 7;\n"
        "  exp = 0;\n"
        "  if (__atomic_compare_exchange_n(&x, &exp, 5, 0, 0, 0)) return 8;\n"
        "  if (exp != 2 || x != 2) return 9;\n"
        "  __atomic_thread_fence(0);\n"
        "  char b = 0;\n"
        "  if (__atomic_test_and_set(&b, 0) != 0 || b != 1) return 10;\n"
        "  if (__atomic_test_and_set(&b, 0) != 1) return 11;\n"
        "  __atomic_clear(&b, 0);\n"
        "  if (b != 0) return 12;\n"
        "  int src = 3, dst = 0;\n"
        "  __atomic_store(&x, &src, 0);\n"
        "  __atomic_load(&x, &dst, 0);\n"
        "  if (x != 3 || dst != 3) return 13;\n"
        "  return 0; }", 0);
}

static void test_alloca_align(void) {
    expect("alloca_align",
        "package main;\n"
        "int main(void) {\n"
        "  char *p = __builtin_alloca_with_align(8, 256);\n"
        "  if (((unsigned long)p & 31) != 0) return 1;\n"
        "  p[0] = 7;\n"
        "  if (p[0] != 7) return 2;\n"
        "  char *q = __builtin_alloca_with_align(1, 1024);\n"
        "  if (((unsigned long)q & 127) != 0) return 3;\n"
        "  char *r = __builtin_alloca(8);\n"
        "  if (((unsigned long)r & 15) != 0) return 4;\n"
        "  char *s = __builtin_alloca_with_align(4, 8);\n"
        "  if (((unsigned long)s & 15) != 0) return 5;\n"
        "  return 0; }", 0);
}

static void test_bsd(void) {
    expect("bsd",
        "package main;\n"
        "int main(void) {\n"
        "  char buf[4]; buf[0] = 1; buf[1] = 2; buf[2] = 3; buf[3] = 4;\n"
        "  __builtin_bcopy(buf, buf + 1, 3);\n"
        "  if (buf[0] != 1 || buf[1] != 1 || buf[2] != 2 || buf[3] != 3) return 1;\n"
        "  char s[3]; s[0] = 5; s[1] = 6; s[2] = 7;\n"
        "  char d[3];\n"
        "  __builtin_bcopy(s, d, 3);\n"
        "  if (d[0] != 5 || d[2] != 7) return 2;\n"
        "  char *p = __builtin_index(\"hello\", 108);\n"
        "  if (!p || p[0] != 108 || p[1] != 108) return 3;\n"
        "  if (__builtin_index(\"hello\", 122)) return 4;\n"
        "  char *q = __builtin_rindex(\"hello\", 108);\n"
        "  if (!q || q[0] != 108 || q[1] != 111) return 5;\n"
        "  if (__builtin_rindex(\"hello\", 122)) return 6;\n"
        "  return 0; }", 0);
}

static void test_case(void) {
    expect("casefold",
        "package main;\n"
        "int main(void) {\n"
        "  if (__builtin_strcasecmp(\"AbC\", \"aBc\") != 0) return 1;\n"
        "  if (__builtin_strcasecmp(\"A\", \"b\") >= 0) return 2;\n"
        "  if (__builtin_strcasecmp(\"b\", \"A\") <= 0) return 3;\n"
        "  if (__builtin_strcasecmp(\"\", \"\") != 0) return 4;\n"
        "  if (__builtin_strncasecmp(\"ABC\", \"ab\", 2) != 0) return 5;\n"
        "  if (__builtin_strncasecmp(\"ABC\", \"ab\", 3) == 0) return 6;\n"
        "  if (__builtin_strncasecmp(\"AB\", \"abc\", 2) != 0) return 7;\n"
        "  if (__builtin_strncasecmp(\"A\", \"b\", 0) != 0) return 8;\n"
        "  return 0; }", 0);
}

static void test_stop(void) {
    expect("stop",
        "package main;\n"
        "int main(void) {\n"
        "  char *p = __builtin_strpbrk(\"hello\", \"lx\");\n"
        "  if (!p || p[0] != 108 || p[1] != 108) return 1;\n"
        "  if (__builtin_strpbrk(\"hello\", \"xyz\")) return 2;\n"
        "  if (__builtin_strpbrk(\"hello\", \"\")) return 3;\n"
        "  if (__builtin_strpbrk(\"\", \"h\")) return 4;\n"
        "  char d[8];\n"
        "  char *q = __builtin_memccpy(d, \"abXcd\", 88, 5);\n"
        "  if (!q || q - d != 3 || d[0] != 97 || d[2] != 88) return 5;\n"
        "  q = __builtin_memccpy(d, \"ab\", 88, 2);\n"
        "  if (q || d[0] != 97 || d[1] != 98) return 6;\n"
        "  q = __builtin_memccpy(d, \"X\", 88, 0);\n"
        "  if (q) return 7;\n"
        "  return 0; }", 0);
}

static void test_rfind(void) {
    expect("rfind",
        "package main;\n"
        "int main(void) {\n"
        "  char *p = __builtin_memrchr(\"abca\", 97, 4);\n"
        "  if (!p || p[0] != 97 || p[-1] != 99) return 1;\n"
        "  if (__builtin_memrchr(\"abca\", 122, 4)) return 2;\n"
        "  if (__builtin_memrchr(\"abca\", 97, 0)) return 3;\n"
        "  char *q = __builtin_strchrnul(\"hello\", 108);\n"
        "  if (!q || q[0] != 108) return 4;\n"
        "  q = __builtin_strchrnul(\"hello\", 122);\n"
        "  if (!q || *q != 0) return 5;\n"
        "  q = __builtin_strchrnul(\"hello\", 0);\n"
        "  if (!q || *q != 0 || q[-1] != 111) return 6;\n"
        "  char *r = __builtin_rawmemchr(\"abX\", 88);\n"
        "  if (!r || r[0] != 88) return 7;\n"
        "  r = __builtin_rawmemchr(\"ab\", 0);\n"
        "  if (!r || *r != 0) return 8;\n"
        "  return 0; }", 0);
}

static void test_rotate(void) {
    expect("rotate",
        "package main;\n"
        "int main(void) {\n"
        "  if ((unsigned)__builtin_rotateleft8(0x81, 1) != 0x03) return 1;\n"
        "  if ((unsigned)__builtin_rotateleft8(0x81, 0) != 0x81) return 2;\n"
        "  if ((unsigned)__builtin_rotateright8(0x81, 1) != 0xc0) return 3;\n"
        "  if ((unsigned)__builtin_rotateleft16(0x8001, 1) != 3) return 4;\n"
        "  if ((unsigned)__builtin_rotateright16(0x8001, 1) != 0xc000) return 5;\n"
        "  if (__builtin_rotateleft32(0x80000000u, 1) != 1u) return 6;\n"
        "  if (__builtin_rotateright32(1u, 1) != 0x80000000u) return 7;\n"
        "  unsigned long long h = 1ull << 63;\n"
        "  if (__builtin_rotateleft64(h, 1) != 1ull) return 8;\n"
        "  if (__builtin_rotateright64(1ull, 1) != h) return 9;\n"
        "  return 0; }", 0);
}

static void test_align(void) {
    expect("align",
        "package main;\n"
        "int main(void) {\n"
        "  if (__builtin_align_down(15, 8) != 8) return 1;\n"
        "  if (__builtin_align_up(15, 8) != 16) return 2;\n"
        "  if (__builtin_align_up(16, 8) != 16) return 3;\n"
        "  if (__builtin_align_down(0, 8) != 0) return 4;\n"
        "  unsigned long n = 100;\n"
        "  if (__builtin_align_up(n, 16) != 112) return 5;\n"
        "  char buf[32];\n"
        "  char *p = buf + 3;\n"
        "  char *q = __builtin_align_up(p, 8);\n"
        "  if (((unsigned long)q & 7) != 0) return 6;\n"
        "  if (q < p || (unsigned long)(q - p) >= 8) return 7;\n"
        "  char *r = __builtin_align_down(p, 8);\n"
        "  if (((unsigned long)r & 7) != 0) return 8;\n"
        "  if ((unsigned long)(p - r) >= 8) return 9;\n"
        "  char *a = __builtin_assume_aligned(buf, 16);\n"
        "  if (a != buf) return 10;\n"
        "  if (__builtin_expect_with_probability(7, 1, 0.5) != 7) return 11;\n"
        "  return 0; }", 0);
}

static void test_funnel(void) {
    expect("funnel",
        "package main;\n"
        "int main(void) {\n"
        "  if (__builtin_fshl(0x00000001u, 0x80000000u, 1) != 3u) return 1;\n"
        "  if (__builtin_fshl(0x12345678u, 0u, 0) != 0x12345678u) return 2;\n"
        "  if (__builtin_fshr(0x00000001u, 0x80000000u, 1) != 0xc0000000u) return 3;\n"
        "  if (__builtin_fshr(0u, 0x12345678u, 0) != 0x12345678u) return 4;\n"
        "  unsigned long long h = 1ull;\n"
        "  if (__builtin_fshl(h, h << 63, 1) != 3ull) return 5;\n"
        "  if ((unsigned)__builtin_bitreverse8(0x01) != 0x80) return 6;\n"
        "  if ((unsigned)__builtin_bitreverse16(0x0001) != 0x8000) return 7;\n"
        "  if (__builtin_bitreverse32(0x00000001u) != 0x80000000u) return 8;\n"
        "  if (__builtin_bitreverse64(1ull) != (1ull << 63)) return 9;\n"
        "  return 0; }", 0);
}

static void test_powi(void) {
    expect("powi",
        "package main;\n"
        "int main(void) {\n"
        "  if (__builtin_powi(2.0, 3) != 8.0) return 1;\n"
        "  if (__builtin_powi(2.0, 0) != 1.0) return 2;\n"
        "  if (__builtin_powi(2.0, -1) != 0.5) return 3;\n"
        "  if (__builtin_powi(2.0, 10) != 1024.0) return 4;\n"
        "  if (__builtin_powif(3.0f, 2) != 9.0f) return 5;\n"
        "  if (__builtin_powil(2.0L, 4) != 16.0L) return 6;\n"
        "  return 0; }", 0);
}

static void test_class(void) {
    expect("class",
        "package main;\n"
        "int main(void) {\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, 0.0) != 5) return 1;\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, 1.0) != 3) return 2;\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, __builtin_inf()) != 2) return 3;\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, __builtin_nan(\"\")) != 1) return 4;\n"
        "  unsigned long long bits = 1;\n"
        "  double sub;\n"
        "  __builtin_memcpy(&sub, &bits, 8);\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, sub) != 4) return 5;\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, 0.0f) != 5) return 6;\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, 1.0f) != 3) return 7;\n"
        "  unsigned fb = 1;\n"
        "  float fsub;\n"
        "  __builtin_memcpy(&fsub, &fb, 4);\n"
        "  if (__builtin_fpclassify(1, 2, 3, 4, 5, fsub) != 4) return 8;\n"
        "  return 0; }", 0);
}

static void test_next(void) {
    expect("next",
        "package main;\n"
        "int main(void) {\n"
        "  double one = 1.0;\n"
        "  double up = __builtin_nextafter(one, 2.0);\n"
        "  double dn = __builtin_nextafter(one, 0.0);\n"
        "  unsigned long long b1, bu, bd;\n"
        "  __builtin_memcpy(&b1, &one, 8);\n"
        "  __builtin_memcpy(&bu, &up, 8);\n"
        "  __builtin_memcpy(&bd, &dn, 8);\n"
        "  if (bu != b1 + 1) return 1;\n"
        "  if (bd != b1 - 1) return 2;\n"
        "  if (__builtin_nextafter(one, one) != one) return 3;\n"
        "  double z = __builtin_nextafter(0.0, 1.0);\n"
        "  unsigned long long bz;\n"
        "  __builtin_memcpy(&bz, &z, 8);\n"
        "  if (bz != 1) return 4;\n"
        "  unsigned long long neg = 1;\n"
        "  neg = neg << 63;\n"
        "  double nz = __builtin_nextafter(0.0, -1.0);\n"
        "  unsigned long long bnz;\n"
        "  __builtin_memcpy(&bnz, &nz, 8);\n"
        "  if (bnz != neg + 1) return 5;\n"
        "  double nzero;\n"
        "  __builtin_memcpy(&nzero, &neg, 8);\n"
        "  double back = __builtin_nextafter(0.0, nzero);\n"
        "  unsigned long long bb;\n"
        "  __builtin_memcpy(&bb, &back, 8);\n"
        "  if (bb != neg) return 6;\n"
        "  double inf = __builtin_inf();\n"
        "  double mx = __builtin_nextafter(inf, 0.0);\n"
        "  unsigned long long bi, bm;\n"
        "  __builtin_memcpy(&bi, &inf, 8);\n"
        "  __builtin_memcpy(&bm, &mx, 8);\n"
        "  if (bm != bi - 1) return 7;\n"
        "  double n = __builtin_nextafter(__builtin_nan(\"\"), 1.0);\n"
        "  if (n == n) return 8;\n"
        "  unsigned long long sb = 1;\n"
        "  double sub;\n"
        "  __builtin_memcpy(&sub, &sb, 8);\n"
        "  if (__builtin_nextafter(sub, 0.0) != 0.0) return 9;\n"
        "  float f1 = 1.0f;\n"
        "  float fu = __builtin_nextafterf(f1, 2.0f);\n"
        "  unsigned u1, uu;\n"
        "  __builtin_memcpy(&u1, &f1, 4);\n"
        "  __builtin_memcpy(&uu, &fu, 4);\n"
        "  if (uu != u1 + 1) return 10;\n"
        "  double lu = __builtin_nextafterl(one, 2.0);\n"
        "  unsigned long long bl;\n"
        "  __builtin_memcpy(&bl, &lu, 8);\n"
        "  if (bl != b1 + 1) return 11;\n"
        "  return 0; }", 0);
}

static void test_stdc(void) {
    expect("stdc",
        "package main;\n"
        "int main(void) {\n"
        "  unsigned char c = 1;\n"
        "  unsigned char z = 0;\n"
        "  if (__builtin_stdc_leading_zeros(c) != 7) return 1;\n"
        "  if (__builtin_stdc_leading_zeros(z) != 8) return 2;\n"
        "  unsigned char h = 0x80;\n"
        "  if (__builtin_stdc_leading_ones(h) != 1) return 3;\n"
        "  unsigned char ff = 0xff;\n"
        "  if (__builtin_stdc_leading_ones(ff) != 8) return 4;\n"
        "  if (__builtin_stdc_trailing_zeros(h) != 7) return 5;\n"
        "  if (__builtin_stdc_trailing_zeros(z) != 8) return 6;\n"
        "  unsigned char lo = 0x0f;\n"
        "  if (__builtin_stdc_trailing_ones(lo) != 4) return 7;\n"
        "  if (__builtin_stdc_trailing_ones(z) != 0) return 8;\n"
        "  unsigned char b3 = 0x08;\n"
        "  if (__builtin_stdc_first_leading_one(b3) != 4) return 9;\n"
        "  if (__builtin_stdc_first_leading_one(z) != 0) return 10;\n"
        "  unsigned char f0 = 0xf0;\n"
        "  if (__builtin_stdc_first_leading_zero(f0) != 4) return 11;\n"
        "  if (__builtin_stdc_first_leading_zero(ff) != 0) return 12;\n"
        "  if (__builtin_stdc_first_trailing_zero(lo) != 5) return 13;\n"
        "  if (__builtin_stdc_first_trailing_zero(ff) != 0) return 14;\n"
        "  if (__builtin_stdc_first_trailing_one(b3) != 4) return 15;\n"
        "  if (__builtin_stdc_first_trailing_one(z) != 0) return 16;\n"
        "  unsigned char sev = 7;\n"
        "  if (__builtin_stdc_count_ones(sev) != 3) return 17;\n"
        "  if (__builtin_stdc_count_zeros(c) != 7) return 18;\n"
        "  if (__builtin_stdc_has_single_bit(b3) != 1) return 19;\n"
        "  if (__builtin_stdc_has_single_bit(sev) != 0) return 20;\n"
        "  if (__builtin_stdc_bit_width(sev) != 3) return 21;\n"
        "  if (__builtin_stdc_bit_width(z) != 0) return 22;\n"
        "  if (__builtin_stdc_bit_floor(sev) != 4) return 23;\n"
        "  if (__builtin_stdc_bit_floor(z) != 0) return 24;\n"
        "  if (__builtin_stdc_bit_ceil(sev) != 8) return 25;\n"
        "  if (__builtin_stdc_bit_ceil(z) != 1) return 26;\n"
        "  if (__builtin_stdc_bit_ceil(c) != 1) return 27;\n"
        "  unsigned char big = 0x81;\n"
        "  if (__builtin_stdc_bit_ceil(big) != 0) return 28;\n"
        "  if (__builtin_stdc_bit_ceil(h) != 0x80) return 29;\n"
        "  if (__builtin_stdc_rotate_left(big, 1) != 0x03) return 30;\n"
        "  if (__builtin_stdc_rotate_right(big, 1) != 0xc0) return 31;\n"
        "  if (__builtin_stdc_rotate_left(big, 8) != 0x81) return 32;\n"
        "  if (__builtin_stdc_leading_zeros(1u) != 31) return 33;\n"
        "  if (__builtin_stdc_leading_zeros(0u) != 32) return 34;\n"
        "  unsigned short s = 1;\n"
        "  if (__builtin_stdc_leading_zeros(s) != 15) return 35;\n"
        "  if (__builtin_stdc_rotate_left(0x80000000u, 1) != 1u) return 36;\n"
        "  unsigned long long top = 1ull << 63;\n"
        "  if (__builtin_stdc_leading_zeros(top) != 0) return 37;\n"
        "  if (__builtin_stdc_leading_zeros(0ull) != 64) return 38;\n"
        "  if (__builtin_stdc_trailing_zeros(top) != 63) return 39;\n"
        "  if (__builtin_stdc_bit_floor(top) != top) return 40;\n"
        "  if (__builtin_stdc_bit_ceil(top | 1ull) != 0ull) return 41;\n"
        "  if (__builtin_stdc_rotate_left(top, 1) != 1ull) return 42;\n"
        "  return 0; }", 0);
}

static void test_bitg(void) {
    expect("bitg",
        "package main;\n"
        "int main(void) {\n"
        "  unsigned char c = 1;\n"
        "  unsigned char z = 0;\n"
        "  unsigned char h = 0x80;\n"
        "  unsigned char ff = 0xff;\n"
        "  if (__builtin_clzg(c) != 7) return 1;\n"
        "  if (__builtin_clzg(z) != 8) return 2;\n"
        "  if (__builtin_clzg(z, 99) != 99) return 3;\n"
        "  if (__builtin_clzg(c, 99) != 7) return 4;\n"
        "  if (__builtin_ctzg(h) != 7) return 5;\n"
        "  if (__builtin_ctzg(z) != 8) return 6;\n"
        "  if (__builtin_ctzg(z, 3) != 3) return 7;\n"
        "  if (__builtin_clzg(1ull) != 63) return 8;\n"
        "  if (__builtin_clzg(0ull) != 64) return 9;\n"
        "  if (__builtin_clzg(0ull, 5) != 5) return 10;\n"
        "  if (__builtin_clrsbg(h) != 0) return 11;\n"
        "  if (__builtin_clrsbg(z) != 7) return 12;\n"
        "  int n = -1;\n"
        "  if (__builtin_clrsbg(n) != 31) return 13;\n"
        "  if (__builtin_clrsbg(1) != 30) return 14;\n"
        "  if (__builtin_ffsg(h) != 8) return 15;\n"
        "  if (__builtin_ffsg(0) != 0) return 16;\n"
        "  if (__builtin_popcountg(ff) != 8) return 17;\n"
        "  if (__builtin_parityg(ff) != 0) return 18;\n"
        "  if (__builtin_parityg(c) != 1) return 19;\n"
        "  unsigned short s = 1;\n"
        "  if (__builtin_clzg(s) != 15) return 20;\n"
        "  if (__builtin_ctzg(0x80000000u) != 31) return 21;\n"
        "  if (__builtin_ctzg(1ull << 40) != 40) return 22;\n"
        "  return 0; }", 0);
}

static void test_lcpy(void) {
    expect("lcpy",
        "package main;\n"
        "int main(void) {\n"
        "  char d[8];\n"
        "  unsigned long n = __builtin_strlcpy(d, \"hi\", 8);\n"
        "  if (n != 2 || d[0] != 104 || d[1] != 105 || d[2] != 0) return 1;\n"
        "  n = __builtin_strlcpy(d, \"hello\", 4);\n"
        "  if (n != 5 || d[0] != 104 || d[1] != 101 || d[2] != 108 || d[3] != 0) return 2;\n"
        "  n = __builtin_strlcpy(d, \"ab\", 0);\n"
        "  if (n != 2 || d[0] != 104) return 3;\n"
        "  n = __builtin_strlcpy(d, \"\", 8);\n"
        "  if (n != 0 || d[0] != 0) return 4;\n"
        "  char e[8]; e[0] = 97; e[1] = 0;\n"
        "  n = __builtin_strlcat(e, \"b\", 8);\n"
        "  if (n != 2 || e[0] != 97 || e[1] != 98 || e[2] != 0) return 5;\n"
        "  n = __builtin_strlcat(e, \"xyz\", 4);\n"
        "  if (n != 5 || e[0] != 97 || e[1] != 98 || e[2] != 120 || e[3] != 0) return 6;\n"
        "  char f[2]; f[0] = 97; f[1] = 98;\n"
        "  n = __builtin_strlcat(f, \"z\", 2);\n"
        "  if (n != 3 || f[0] != 97) return 7;\n"
        "  char s[6]; s[0] = 97; s[1] = 44; s[2] = 98; s[3] = 44; s[4] = 99; s[5] = 0;\n"
        "  char *p = s;\n"
        "  char *t = __builtin_strsep(&p, \",\");\n"
        "  if (!t || t[0] != 97 || t[1] != 0 || !p || p[0] != 98) return 8;\n"
        "  t = __builtin_strsep(&p, \",\");\n"
        "  if (!t || t[0] != 98 || !p || p[0] != 99) return 9;\n"
        "  t = __builtin_strsep(&p, \",\");\n"
        "  if (!t || t[0] != 99 || p) return 10;\n"
        "  if (__builtin_strsep(&p, \",\")) return 11;\n"
        "  char u[3]; u[0] = 44; u[1] = 97; u[2] = 0;\n"
        "  char *q = u;\n"
        "  t = __builtin_strsep(&q, \",\");\n"
        "  if (!t || t[0] != 0 || !q || q[0] != 97) return 12;\n"
        "  return 0; }", 0);
}

static void test_normal(void) {
    expect("normal",
        "package main;\n"
        "int main(void) {\n"
        "  if (!__builtin_isnormal(1.0)) return 1;\n"
        "  if (__builtin_isnormal(0.0)) return 2;\n"
        "  if (!__builtin_isnormal(-2.0)) return 3;\n"
        "  if (__builtin_isnormal(__builtin_inf())) return 4;\n"
        "  if (__builtin_isnormal(__builtin_nan(\"\"))) return 5;\n"
        "  unsigned long long b = 1;\n"
        "  double sub;\n"
        "  __builtin_memcpy(&sub, &b, 8);\n"
        "  if (__builtin_isnormal(sub)) return 6;\n"
        "  float f = 1.0f;\n"
        "  if (!__builtin_isnormal(f)) return 7;\n"
        "  float zf = 0.0f;\n"
        "  if (__builtin_isnormal(zf)) return 8;\n"
        "  unsigned fb = 1;\n"
        "  float fsub;\n"
        "  __builtin_memcpy(&fsub, &fb, 4);\n"
        "  if (__builtin_isnormal(fsub)) return 9;\n"
        "  if (!__builtin_isnormalf(f)) return 10;\n"
        "  if (!__builtin_isnormall(-2.0)) return 11;\n"
        "  return 0; }", 0);
}

static void test_sat(void) {
    expect("sat",
        "package main;\n"
        "int main(void) {\n"
        "  unsigned char a = 200, b = 100;\n"
        "  if (__builtin_add_sat(a, b) != 255) return 1;\n"
        "  unsigned char c1 = 1, c2 = 2;\n"
        "  if (__builtin_add_sat(c1, c2) != 3) return 2;\n"
        "  unsigned char z = 0, one = 1, five = 5, three = 3;\n"
        "  if (__builtin_sub_sat(z, one) != 0) return 3;\n"
        "  if (__builtin_sub_sat(five, three) != 2) return 4;\n"
        "  signed char p = 100, n = -100;\n"
        "  if (__builtin_add_sat(p, p) != 127) return 5;\n"
        "  if (__builtin_add_sat(n, n) != -128) return 6;\n"
        "  if (__builtin_sub_sat(n, p) != -128) return 7;\n"
        "  if (__builtin_sub_sat(p, n) != 127) return 8;\n"
        "  signed char ten = 10, twenty = 20;\n"
        "  if (__builtin_mul_sat(ten, twenty) != 127) return 9;\n"
        "  unsigned char u16 = 16;\n"
        "  if (__builtin_mul_sat(u16, u16) != 255) return 10;\n"
        "  unsigned short h = 60000;\n"
        "  if (__builtin_add_sat(h, h) != 65535) return 11;\n"
        "  unsigned u = 4294967295u;\n"
        "  unsigned u1 = 1, u2 = 2;\n"
        "  if (__builtin_add_sat(u, u1) != u) return 12;\n"
        "  if (__builtin_sub_sat(u1, u2) != 0u) return 13;\n"
        "  if (__builtin_mul_sat(u, u2) != u) return 14;\n"
        "  int s = 2147483647;\n"
        "  int si = 1;\n"
        "  if (__builtin_add_sat(s, si) != s) return 15;\n"
        "  int t = -2147483647 - 1;\n"
        "  if (__builtin_sub_sat(t, si) != t) return 16;\n"
        "  if (__builtin_mul_sat(s, si + si) != s) return 17;\n"
        "  int neg1 = -1;\n"
        "  if (__builtin_mul_sat(t, neg1) != s) return 18;\n"
        "  if (__builtin_add_sat(si, si + si) != 3) return 19;\n"
        "  unsigned long long U = ~0ull;\n"
        "  unsigned long long U1 = 1, U2 = 2;\n"
        "  if (__builtin_add_sat(U, U1) != U) return 20;\n"
        "  if (__builtin_mul_sat(U, U2) != U) return 21;\n"
        "  long long S = (long long)(1ull << 63);\n"
        "  long long Sm1 = -1;\n"
        "  long long lone = 1;\n"
        "  if (__builtin_sub_sat(S, lone) != S) return 22;\n"
        "  long long P = 9223372036854775807ll;\n"
        "  if (__builtin_add_sat(P, lone) != P) return 23;\n"
        "  if (__builtin_mul_sat(P, (long long)U2) != P) return 24;\n"
        "  if (__builtin_mul_sat(S, Sm1) != P) return 25;\n"
        "  if (__builtin_mul_sat(lone, (long long)U2) != 2) return 26;\n"
        "  return 0; }", 0);
}

static void test_nans(void) {
    expect("nans",
        "package main;\n"
        "const double gs = __builtin_nans(\"1\");\n"
        "int main(void) {\n"
        "  unsigned long long b;\n"
        "  double d = __builtin_nans(\"\");\n"
        "  __builtin_memcpy(&b, &d, 8);\n"
        "  if (b != 0x7ff4000000000000ull) return 1;\n"
        "  double p = __builtin_nans(\"42\");\n"
        "  __builtin_memcpy(&b, &p, 8);\n"
        "  if (b != 0x7ff000000000002aull) return 2;\n"
        "  float f = __builtin_nansf(\"\");\n"
        "  unsigned fb = 0;\n"
        "  __builtin_memcpy(&fb, &f, 4);\n"
        "  if (fb != 0x7fa00000u) return 3;\n"
        "  float pf = __builtin_nansf(\"1\");\n"
        "  __builtin_memcpy(&fb, &pf, 4);\n"
        "  if (fb != 0x7f800001u) return 4;\n"
        "  long double l = __builtin_nansl(\"\");\n"
        "  __builtin_memcpy(&b, &l, 8);\n"
        "  if (b != 0x7ff4000000000000ull) return 5;\n"
        "  __builtin_memcpy(&b, &gs, 8);\n"
        "  if (b != 0x7ff0000000000001ull) return 6;\n"
        "  double q = __builtin_nans(\"0x8000000000001\");\n"
        "  __builtin_memcpy(&b, &q, 8);\n"
        "  if (b != 0x7ff0000000000001ull) return 7;\n"
        "  return 0; }", 0);
}

static void test_copy(void) {
    expect("copy",
        "package main;\n"
        "int main(void) {\n"
        "  char d[8];\n"
        "  char *p = __builtin_strcpy(d, \"hi\");\n"
        "  if (p != d || d[0] != 104 || d[1] != 105 || d[2] != 0) return 1;\n"
        "  p = __builtin_stpcpy(d, \"ab\");\n"
        "  if (p != d + 2 || d[2] != 0) return 2;\n"
        "  char e[4]; e[0] = 9; e[1] = 9; e[2] = 9; e[3] = 9;\n"
        "  __builtin_strncpy(e, \"ab\", 4);\n"
        "  if (e[0] != 97 || e[1] != 98 || e[2] != 0 || e[3] != 0) return 3;\n"
        "  e[0] = 9; e[1] = 9; e[2] = 9; e[3] = 9;\n"
        "  __builtin_strncpy(e, \"abcd\", 3);\n"
        "  if (e[0] != 97 || e[1] != 98 || e[2] != 99 || e[3] != 9) return 4;\n"
        "  char g[8]; g[0] = 120; g[1] = 0;\n"
        "  p = __builtin_strcat(g, \"yz\");\n"
        "  if (p != g || g[0] != 120 || g[1] != 121 || g[2] != 122 || g[3] != 0)\n"
        "    return 5;\n"
        "  g[0] = 120; g[1] = 0;\n"
        "  __builtin_strncat(g, \"yz\", 1);\n"
        "  if (g[0] != 120 || g[1] != 121 || g[2] != 0) return 6;\n"
        "  char s[] = {97, 98, 99, 100, 0};\n"
        "  if (__builtin_strstr(s, \"cd\") != s + 2) return 7;\n"
        "  if (__builtin_strstr(s, \"z\")) return 8;\n"
        "  if (__builtin_strstr(s, \"\") != s) return 9;\n"
        "  return 0; }", 0);
}

static void test_trap(void) {
    /* brk #1 → SIGTRAP, same status clang produces. */
    expect("trap",
        "package main;\n"
        "int main(void) { __builtin_trap(); return 7; }", 133);
    expect("unreach",
        "package main;\n"
        "int main(void) { __builtin_unreachable(); return 1; }", 133);
    expect("trap_skip",
        "package main;\n"
        "int main(void) { if (0) __builtin_trap(); return 7; }", 7);
    expect("debugtrap",
        "package main;\n"
        "int main(void) { __builtin_debugtrap(); return 7; }", 133);
    expect("debugtrap_skip",
        "package main;\n"
        "int main(void) { if (0) __builtin_debugtrap(); return 7; }", 7);
}

static void test_hint(void) {
    expect("hint",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 1;\n"
        "  __builtin_assume(x = 2);\n"
        "  if (x != 2) return 1;\n"
        "  if (__builtin_unpredictable(7) != 7) return 2;\n"
        "  int z = 3;\n"
        "  if (__builtin_unpredictable(z = 4) != 4 || z != 4) return 3;\n"
        "  char buf[4];\n"
        "  char *p = __builtin_unpredictable(buf);\n"
        "  if (p != buf) return 4;\n"
        "  return 0; }", 0);
}

static void test_cache(void) {
    expect("cache",
        "package main;\n"
        "int main(void) {\n"
        "  char b[128];\n"
        "  __builtin_clear_cache(b, b);\n"
        "  __builtin_clear_cache(b + 64, b);\n"
        "  __builtin_clear_cache(b, b + 64);\n"
        "  return 7; }", 7);
}

static void test_cycle(void) {
    expect("cycle",
        "package main;\n"
        "int main(void) {\n"
        "  unsigned long long a = __builtin_readcyclecounter();\n"
        "  unsigned long long b = __builtin_readcyclecounter();\n"
        "  if (a == 0) return 1;\n"
        "  if (b < a) return 2;\n"
        "  return 0; }", 0);
}

static void test_barrier(void) {
    expect("barrier",
        "package main;\n"
        "int main(void) {\n"
        "  __builtin_arm_dmb(11);\n"
        "  __builtin_arm_dsb(15);\n"
        "  __builtin_arm_isb(15);\n"
        "  return 7; }", 7);
}

static void test_fence(void) {
    expect("fence",
        "package main;\n"
        "int main(void) {\n"
        "  __atomic_thread_fence(0);\n"
        "  __atomic_thread_fence(2);\n"
        "  __atomic_thread_fence(3);\n"
        "  __atomic_thread_fence(5);\n"
        "  __atomic_signal_fence(5);\n"
        "  __sync_synchronize();\n"
        "  return 7; }", 7);
}

static void test_aload(void) {
    expect("aload",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 1;\n"
        "  if (__atomic_load_n(&x, 0) != 1) return 1;\n"
        "  if (__atomic_load_n(&x, 2) != 1) return 2;\n"
        "  if (__atomic_load_n(&x, 5) != 1) return 3;\n"
        "  x = -3;\n"
        "  if (__atomic_load_n(&x, 2) != -3) return 4;\n"
        "  __atomic_store_n(&x, 4, 0);\n"
        "  if (x != 4) return 5;\n"
        "  __atomic_store_n(&x, 9, 3);\n"
        "  if (__atomic_load_n(&x, 5) != 9) return 6;\n"
        "  long long y = 3;\n"
        "  if (__atomic_load_n(&y, 2) != 3) return 7;\n"
        "  __atomic_store_n(&y, 8, 5);\n"
        "  if (y != 8) return 8;\n"
        "  unsigned char c = 1;\n"
        "  if (__atomic_load_n(&c, 2) != 1) return 9;\n"
        "  __atomic_store_n(&c, 6, 3);\n"
        "  if (c != 6) return 10;\n"
        "  unsigned short h = 2;\n"
        "  if (__atomic_load_n(&h, 2) != 2) return 11;\n"
        "  __atomic_store_n(&h, 7, 3);\n"
        "  if (h != 7) return 12;\n"
        "  signed char sc = -5;\n"
        "  if (__atomic_load_n(&sc, 5) != -5) return 13;\n"
        "  return 0; }", 0);
}

static void test_aptr(void) {
    expect("aptr",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 1;\n"
        "  int r = 0;\n"
        "  __atomic_load(&x, &r, 0);\n"
        "  if (r != 1) return 1;\n"
        "  __atomic_load(&x, &r, 2);\n"
        "  if (r != 1) return 2;\n"
        "  __atomic_load(&x, &r, 5);\n"
        "  if (r != 1) return 3;\n"
        "  x = -3;\n"
        "  __atomic_load(&x, &r, 2);\n"
        "  if (r != -3) return 4;\n"
        "  int v = 4;\n"
        "  __atomic_store(&x, &v, 0);\n"
        "  if (x != 4) return 5;\n"
        "  v = 9;\n"
        "  __atomic_store(&x, &v, 3);\n"
        "  __atomic_load(&x, &r, 5);\n"
        "  if (r != 9) return 6;\n"
        "  long long y = 3;\n"
        "  long long ry = 0;\n"
        "  __atomic_load(&y, &ry, 2);\n"
        "  if (ry != 3) return 7;\n"
        "  long long vy = 8;\n"
        "  __atomic_store(&y, &vy, 5);\n"
        "  if (y != 8) return 8;\n"
        "  unsigned char c = 1;\n"
        "  unsigned char rc = 0;\n"
        "  __atomic_load(&c, &rc, 2);\n"
        "  if (rc != 1) return 9;\n"
        "  unsigned char vc = 6;\n"
        "  __atomic_store(&c, &vc, 3);\n"
        "  if (c != 6) return 10;\n"
        "  unsigned short h = 2;\n"
        "  unsigned short rh = 0;\n"
        "  __atomic_load(&h, &rh, 2);\n"
        "  if (rh != 2) return 11;\n"
        "  unsigned short vh = 7;\n"
        "  __atomic_store(&h, &vh, 3);\n"
        "  if (h != 7) return 12;\n"
        "  signed char sc = -5;\n"
        "  signed char rsc = 0;\n"
        "  __atomic_load(&sc, &rsc, 5);\n"
        "  if (rsc != -5) return 13;\n"
        "  return 0; }", 0);
}

static void test_xchg(void) {
    expect("xchg",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 9;\n"
        "  if (__atomic_exchange_n(&x, 4, 0) != 9) return 1;\n"
        "  if (x != 4) return 2;\n"
        "  if (__atomic_exchange_n(&x, 1, 2) != 4) return 3;\n"
        "  if (__atomic_exchange_n(&x, 6, 3) != 1) return 4;\n"
        "  if (__atomic_exchange_n(&x, 8, 5) != 6) return 5;\n"
        "  if (x != 8) return 6;\n"
        "  x = -3;\n"
        "  if (__atomic_exchange_n(&x, 2, 5) != -3) return 7;\n"
        "  long long y = 3;\n"
        "  if (__atomic_exchange_n(&y, 8, 3) != 3) return 8;\n"
        "  if (y != 8) return 9;\n"
        "  unsigned char c = 1;\n"
        "  if (__atomic_exchange_n(&c, 6, 2) != 1) return 10;\n"
        "  if (c != 6) return 11;\n"
        "  unsigned short h = 2;\n"
        "  if (__atomic_exchange_n(&h, 7, 3) != 2) return 12;\n"
        "  if (h != 7) return 13;\n"
        "  signed char sc = -5;\n"
        "  if (__atomic_exchange_n(&sc, 4, 5) != -5) return 14;\n"
        "  if (sc != 4) return 15;\n"
        "  return 0; }", 0);
}

static void test_xchgp(void) {
    expect("xchgp",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 9;\n"
        "  int v = 4;\n"
        "  int r = 0;\n"
        "  __atomic_exchange(&x, &v, &r, 0);\n"
        "  if (r != 9 || x != 4) return 1;\n"
        "  v = 1;\n"
        "  __atomic_exchange(&x, &v, &r, 2);\n"
        "  if (r != 4 || x != 1) return 2;\n"
        "  v = 6;\n"
        "  __atomic_exchange(&x, &v, &r, 3);\n"
        "  if (r != 1 || x != 6) return 3;\n"
        "  v = 8;\n"
        "  __atomic_exchange(&x, &v, &r, 5);\n"
        "  if (r != 6 || x != 8) return 4;\n"
        "  x = -3;\n"
        "  v = 2;\n"
        "  __atomic_exchange(&x, &v, &r, 5);\n"
        "  if (r != -3 || x != 2) return 5;\n"
        "  long long y = 3;\n"
        "  long long vy = 8;\n"
        "  long long ry = 0;\n"
        "  __atomic_exchange(&y, &vy, &ry, 3);\n"
        "  if (ry != 3 || y != 8) return 6;\n"
        "  unsigned char c = 1;\n"
        "  unsigned char vc = 6;\n"
        "  unsigned char rc = 0;\n"
        "  __atomic_exchange(&c, &vc, &rc, 2);\n"
        "  if (rc != 1 || c != 6) return 7;\n"
        "  unsigned short h = 2;\n"
        "  unsigned short vh = 7;\n"
        "  unsigned short rh = 0;\n"
        "  __atomic_exchange(&h, &vh, &rh, 3);\n"
        "  if (rh != 2 || h != 7) return 8;\n"
        "  signed char sc = -5;\n"
        "  signed char vsc = 4;\n"
        "  signed char rsc = 0;\n"
        "  __atomic_exchange(&sc, &vsc, &rsc, 5);\n"
        "  if (rsc != -5 || sc != 4) return 9;\n"
        "  return 0; }", 0);
}

static void test_fadd(void) {
    expect("fadd",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 9;\n"
        "  if (__atomic_fetch_add(&x, 4, 0) != 9) return 1;\n"
        "  if (x != 13) return 2;\n"
        "  if (__atomic_fetch_add(&x, 1, 2) != 13) return 3;\n"
        "  if (__atomic_add_fetch(&x, 2, 3) != 16) return 4;\n"
        "  if (__atomic_fetch_sub(&x, 6, 5) != 16) return 5;\n"
        "  if (x != 10) return 6;\n"
        "  x = -3;\n"
        "  if (__atomic_add_fetch(&x, 1, 0) != -2) return 7;\n"
        "  long long y = 3;\n"
        "  if (__atomic_fetch_add(&y, 8, 2) != 3) return 8;\n"
        "  if (y != 11) return 9;\n"
        "  unsigned char c = 250;\n"
        "  if (__atomic_fetch_add(&c, 10, 5) != 250) return 10;\n"
        "  if (c != 4) return 11;\n"
        "  c = 200;\n"
        "  if (__atomic_add_fetch(&c, 100, 0) != 44) return 12;\n"
        "  unsigned short h = 2;\n"
        "  if (__atomic_fetch_sub(&h, 3, 3) != 2) return 13;\n"
        "  if (h != 65535) return 14;\n"
        "  signed char sc = 127;\n"
        "  if (__atomic_add_fetch(&sc, 1, 5) != -128) return 15;\n"
        "  if (__atomic_sub_fetch(&sc, 1, 0) != 127) return 16;\n"
        "  return 0; }", 0);
}

static void test_bitrmw(void) {
    expect("bitrmw",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 15;\n"
        "  if (__atomic_fetch_and(&x, 51, 0) != 15) return 1;\n"
        "  if (x != 3) return 2;\n"
        "  if (__atomic_fetch_or(&x, 16, 2) != 3) return 3;\n"
        "  if (x != 19) return 4;\n"
        "  if (__atomic_or_fetch(&x, 4, 0) != 23) return 5;\n"
        "  if (__atomic_fetch_xor(&x, 7, 3) != 23) return 6;\n"
        "  if (x != 16) return 7;\n"
        "  if (__atomic_xor_fetch(&x, 1, 5) != 17) return 8;\n"
        "  x = -1;\n"
        "  if (__atomic_and_fetch(&x, 15, 5) != 15) return 9;\n"
        "  unsigned char c = 255;\n"
        "  if (__atomic_fetch_and(&c, 15, 5) != 255) return 10;\n"
        "  if (c != 15) return 11;\n"
        "  if (__atomic_or_fetch(&c, 240, 0) != 255) return 12;\n"
        "  signed char sc = -1;\n"
        "  if (__atomic_fetch_xor(&sc, 1, 5) != -1) return 13;\n"
        "  if (sc != -2) return 14;\n"
        "  unsigned short h = 255;\n"
        "  if (__atomic_fetch_or(&h, 3840, 3) != 255) return 15;\n"
        "  if (h != 4095) return 16;\n"
        "  long long y = 1;\n"
        "  if (__atomic_fetch_xor(&y, 3, 2) != 1) return 17;\n"
        "  if (y != 2) return 18;\n"
        "  return 0; }", 0);
}

static void test_cas(void) {
    expect("cas",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 1;\n"
        "  int e = 1;\n"
        "  if (!__atomic_compare_exchange_n(&x, &e, 4, 0, 0, 0) || x != 4) return 1;\n"
        "  e = 0;\n"
        "  if (__atomic_compare_exchange_n(&x, &e, 5, 0, 2, 2) || e != 4 || x != 4) return 2;\n"
        "  e = 4;\n"
        "  if (!__atomic_compare_exchange_n(&x, &e, 9, 0, 3, 0) || x != 9) return 3;\n"
        "  e = 9;\n"
        "  if (!__atomic_compare_exchange_n(&x, &e, 8, 0, 5, 5) || x != 8) return 4;\n"
        "  x = -3;\n"
        "  e = -3;\n"
        "  if (!__atomic_compare_exchange_n(&x, &e, 2, 0, 5, 5) || x != 2) return 5;\n"
        "  long long y = 3;\n"
        "  long long ey = 3;\n"
        "  if (!__atomic_compare_exchange_n(&y, &ey, 8, 0, 2, 0) || y != 8) return 6;\n"
        "  unsigned char c = 1;\n"
        "  unsigned char ec = 1;\n"
        "  if (!__atomic_compare_exchange_n(&c, &ec, 6, 0, 0, 0) || c != 6) return 7;\n"
        "  ec = 0;\n"
        "  if (__atomic_compare_exchange_n(&c, &ec, 9, 0, 5, 5) || ec != 6) return 8;\n"
        "  unsigned short h = 2;\n"
        "  unsigned short eh = 2;\n"
        "  if (!__atomic_compare_exchange_n(&h, &eh, 7, 1, 3, 0) || h != 7) return 9;\n"
        "  signed char sc = -5;\n"
        "  signed char esc = -5;\n"
        "  if (!__atomic_compare_exchange_n(&sc, &esc, 4, 0, 5, 0) || sc != 4) return 10;\n"
        "  esc = 0;\n"
        "  if (__atomic_compare_exchange_n(&sc, &esc, 1, 0, 0, 0) || esc != 4) return 11;\n"
        "  int d = 3;\n"
        "  x = 2;\n"
        "  e = 2;\n"
        "  if (!__atomic_compare_exchange(&x, &e, &d, 0, 5, 5) || x != 3) return 12;\n"
        "  return 0; }", 0);
}

static void test_tas(void) {
    expect("tas",
        "package main;\n"
        "int main(void) {\n"
        "  char b = 0;\n"
        "  if (__atomic_test_and_set(&b, 0) != 0 || b != 1) return 1;\n"
        "  if (__atomic_test_and_set(&b, 2) != 1) return 2;\n"
        "  __atomic_clear(&b, 0);\n"
        "  if (b != 0) return 3;\n"
        "  b = 2;\n"
        "  if (__atomic_test_and_set(&b, 5) != 1 || b != 1) return 4;\n"
        "  __atomic_clear(&b, 3);\n"
        "  if (b != 0) return 5;\n"
        "  __atomic_clear(&b, 5);\n"
        "  if (b != 0) return 6;\n"
        "  if (__atomic_test_and_set(&b, 3) != 0 || b != 1) return 7;\n"
        "  return 0; }", 0);
}

static void test_sync(void) {
    expect("sync",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 9;\n"
        "  if (__sync_fetch_and_add(&x, 4) != 9 || x != 13) return 1;\n"
        "  if (__sync_add_and_fetch(&x, 1) != 14) return 2;\n"
        "  if (__sync_fetch_and_sub(&x, 4) != 14 || x != 10) return 3;\n"
        "  if (__sync_fetch_and_and(&x, 7) != 10 || x != 2) return 4;\n"
        "  if (__sync_fetch_and_or(&x, 8) != 2 || x != 10) return 5;\n"
        "  if (__sync_xor_and_fetch(&x, 1) != 11) return 6;\n"
        "  if (__sync_lock_test_and_set(&x, 3) != 11 || x != 3) return 7;\n"
        "  __sync_lock_release(&x);\n"
        "  if (x != 0) return 8;\n"
        "  unsigned char c = 250;\n"
        "  if (__sync_fetch_and_add(&c, 10) != 250 || c != 4) return 9;\n"
        "  signed char sc = 127;\n"
        "  if (__sync_add_and_fetch(&sc, 1) != -128) return 10;\n"
        "  long long y = 1;\n"
        "  if (__sync_fetch_and_xor(&y, 3) != 1 || y != 2) return 11;\n"
        "  __sync_lock_release(&y);\n"
        "  if (y != 0) return 12;\n"
        "  return 0; }", 0);
}

static void test_scas(void) {
    expect("scas",
        "package main;\n"
        "int main(void) {\n"
        "  int x = 9;\n"
        "  if (__sync_bool_compare_and_swap(&x, 8, 1) != 0 || x != 9) return 1;\n"
        "  if (__sync_bool_compare_and_swap(&x, 9, 4) != 1 || x != 4) return 2;\n"
        "  if (__sync_val_compare_and_swap(&x, 3, 7) != 4 || x != 4) return 3;\n"
        "  if (__sync_val_compare_and_swap(&x, 4, 11) != 4 || x != 11) return 4;\n"
        "  unsigned char c = 250;\n"
        "  if (__sync_bool_compare_and_swap(&c, 249, 1) != 0 || c != 250) return 5;\n"
        "  if (__sync_val_compare_and_swap(&c, 250, 3) != 250 || c != 3) return 6;\n"
        "  signed char sc = -1;\n"
        "  if (__sync_val_compare_and_swap(&sc, 0, 2) != -1 || sc != -1) return 7;\n"
        "  if (__sync_bool_compare_and_swap(&sc, -1, 5) != 1 || sc != 5) return 8;\n"
        "  short h = 7;\n"
        "  if (__sync_val_compare_and_swap(&h, 7, -2) != 7 || h != -2) return 9;\n"
        "  long long y = 1;\n"
        "  if (__sync_val_compare_and_swap(&y, 1, 9) != 1 || y != 9) return 10;\n"
        "  if (__sync_bool_compare_and_swap(&y, 8, 2) != 0 || y != 9) return 11;\n"
        "  return 0; }", 0);
}

static void test_fat(void) {
    expect("fat",
        "package main;\n"
        "int main(void) {\n"
        "  float f = 0;\n"
        "  __atomic_store_n(&f, -1.5f, 5);\n"
        "  if (*(int *)&f != 0xbfc00000) return 1;\n"
        "  if (__atomic_load_n(&f, 5) != -1.5f) return 2;\n"
        "  __atomic_store_n(&f, 3.0f, 0);\n"
        "  if (__atomic_load_n(&f, 0) != 3.0f) return 3;\n"
        "  float out = 0;\n"
        "  __atomic_load(&f, &out, 2);\n"
        "  if (out != 3.0f) return 4;\n"
        "  float in = 4.5f;\n"
        "  __atomic_store(&f, &in, 3);\n"
        "  if (*(int *)&f != 0x40900000) return 5;\n"
        "  double d = 0;\n"
        "  __atomic_store_n(&d, -2.25, 5);\n"
        "  if (*(long long *)&d != 0xc002000000000000LL) return 6;\n"
        "  if (__atomic_load_n(&d, 2) != -2.25) return 7;\n"
        "  return 0; }", 0);
}

static void test_fxchg(void) {
    expect("fxchg",
        "package main;\n"
        "int main(void) {\n"
        "  float f = -1.5f;\n"
        "  float old = __atomic_exchange_n(&f, 4.5f, 5);\n"
        "  if (*(int *)&old != 0xbfc00000) return 1;\n"
        "  if (*(int *)&f != 0x40900000) return 2;\n"
        "  float neu = 1.0f;\n"
        "  float got = 0;\n"
        "  __atomic_exchange(&f, &neu, &got, 2);\n"
        "  if (*(int *)&got != 0x40900000) return 3;\n"
        "  if (*(int *)&f != 0x3f800000) return 4;\n"
        "  double d = -2.25;\n"
        "  double od = __atomic_exchange_n(&d, 2.5, 0);\n"
        "  if (*(long long *)&od != 0xc002000000000000LL) return 5;\n"
        "  if (*(long long *)&d != 0x4004000000000000LL) return 6;\n"
        "  return 0; }", 0);
}

static void test_fcas(void) {
    expect("fcas",
        "package main;\n"
        "int main(void) {\n"
        "  float f = -1.5f;\n"
        "  float exp = -1.5f;\n"
        "  if (__atomic_compare_exchange_n(&f, &exp, 4.5f, 0, 5, 5) != 1) return 1;\n"
        "  if (*(int *)&f != 0x40900000) return 2;\n"
        "  exp = 1.0f;\n"
        "  if (__atomic_compare_exchange_n(&f, &exp, 2.0f, 0, 5, 5) != 0) return 3;\n"
        "  if (*(int *)&exp != 0x40900000 || *(int *)&f != 0x40900000) return 4;\n"
        "  float neu = 1.0f;\n"
        "  float want = 4.5f;\n"
        "  if (__atomic_compare_exchange(&f, &want, &neu, 0, 2, 2) != 1) return 5;\n"
        "  if (*(int *)&f != 0x3f800000) return 6;\n"
        "  double d = -2.25;\n"
        "  double de = 0;\n"
        "  if (__atomic_compare_exchange_n(&d, &de, 2.5, 0, 0, 0) != 0) return 7;\n"
        "  if (*(long long *)&de != 0xc002000000000000LL) return 8;\n"
        "  if (*(long long *)&d != 0xc002000000000000LL) return 9;\n"
        "  de = -2.25;\n"
        "  if (__atomic_compare_exchange_n(&d, &de, 2.5, 0, 0, 0) != 1) return 10;\n"
        "  if (*(long long *)&d != 0x4004000000000000LL) return 11;\n"
        "  return 0; }", 0);
}

static void test_fpadd(void) {
    expect("fpadd",
        "package main;\n"
        "int main(void) {\n"
        "  float f = 1.5f;\n"
        "  float old = __atomic_fetch_add(&f, 1.0f, 5);\n"
        "  if (*(int *)&old != 0x3fc00000) return 1;\n"
        "  if (*(int *)&f != 0x40200000) return 2;\n"
        "  if (__atomic_add_fetch(&f, 0.5f, 2) != 3.0f) return 3;\n"
        "  if (__atomic_sub_fetch(&f, 1.0f, 0) != 2.0f) return 4;\n"
        "  double d = 2.25;\n"
        "  if (__atomic_fetch_sub(&d, 0.25, 5) != 2.25) return 5;\n"
        "  if (d != 2.0) return 6;\n"
        "  if (__atomic_fetch_add(&d, 0.5, 3) != 2.0) return 7;\n"
        "  if (d != 2.5) return 8;\n"
        "  return 0; }", 0);
}

static void test_zfill(void) {
    expect("zfill",
        "package main;\n"
        "static int z[5000];\n"
        "static const int r[5000] = { 1, 2, 3 };\n"
        "int g __attribute__((aligned(64)));\n"
        "int main(void) {\n"
        "  if (((unsigned long)&g) & 63) return 1;\n"
        "  if (z[0] != 0 || z[4095] != 0 || z[4096] != 0 || z[4999] != 0) return 2;\n"
        "  z[0] = 7;\n"
        "  z[4096] = 8;\n"
        "  z[4999] = 9;\n"
        "  if (z[0] != 7 || z[4096] != 8 || z[4999] != 9) return 3;\n"
        "  if (r[0] != 1 || r[1] != 2 || r[2] != 3) return 4;\n"
        "  if (r[4096] != 0 || r[4999] != 0) return 5;\n"
        "  g = 6;\n"
        "  if (g != 6) return 6;\n"
        "  return 0; }", 0);
}

static void test_int128(void) {
    expect("i128div",
        "package main;\n"
        "int main(void) {\n"
        "  unsigned __int128 u = (unsigned __int128)100;\n"
        "  if ((int)(u / 7) != 14) return 1;\n"
        "  if ((int)(u % 7) != 2) return 2;\n"
        "  __int128 s = -20;\n"
        "  if ((int)(s / 3) != -6) return 3;\n"
        "  if ((int)(s % 3) != -2) return 4;\n"
        "  unsigned __int128 h = (unsigned __int128)1 << 64;\n"
        "  h = h / 2;\n"
        "  if ((int)(h >> 63) != 1) return 5;\n"
        "  return 0; }", 0);
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
    test_float();
    test_vec16();
    test_int128();
    test_setjmp();
    test_bitops();
    test_fpmath();
    test_scan();
    test_find();
    test_copy();
    test_pad();
    test_span();
    test_atomic();
    test_alloca_align();
    test_bsd();
    test_case();
    test_stop();
    test_rfind();
    test_rotate();
    test_align();
    test_funnel();
    test_powi();
    test_class();
    test_next();
    test_stdc();
    test_bitg();
    test_lcpy();
    test_normal();
    test_sat();
    test_nans();
    test_trap();
    test_hint();
    test_cache();
    test_cycle();
    test_barrier();
    test_fence();
    test_aload();
    test_aptr();
    test_xchg();
    test_xchgp();
    test_fadd();
    test_bitrmw();
    test_cas();
    test_tas();
    test_sync();
    test_scas();
    test_fat();
    test_fxchg();
    test_fcas();
    test_fpadd();
    test_zfill();
    test_syscall();
    test_frame_addr();
    test_macho_obj();
    test_common();
    test_weak();
    test_wref();
    test_alias();
    test_hidden();
    test_addend();
    test_used();
    test_subtractor();
    test_got();
    test_fn_got();
    test_call_stub();
    test_tls();
    test_malloc();
    test_strdup();
    test_io();
    test_getenv();
    test_macho_link();
    return t_finalize();
}

#else

int main(void) { return t_finalize(); }  /* non-Apple-Silicon: no-op */

#endif
