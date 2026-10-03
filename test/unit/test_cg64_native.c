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
    T_ASSERT_EQ_INT((int)xnreloc, 1);

    EmitModule back;
    T_ASSERT_EQ_INT(emit_obj_read(xpath, &back), 0);
    T_ASSERT(back.text.len > 0);
    T_ASSERT_EQ_INT((int)back.num_relocs, 1);
    T_ASSERT(emit_module_find_symbol(&back, "call") >= 0);
    int other = emit_module_find_symbol(&back, "other");
    T_ASSERT(other >= 0);
    T_ASSERT_EQ_INT(back.syms[other].shndx, 0);
    T_ASSERT_EQ_INT((int)back.relocs[0].type, 2);
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
    test_trap();
    test_syscall();
    test_frame_addr();
    test_macho_obj();
    test_macho_link();
    return t_finalize();
}

#else

int main(void) { return t_finalize(); }  /* non-Apple-Silicon: no-op */

#endif
