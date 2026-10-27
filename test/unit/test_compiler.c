#include "fakecc/common.h"
#include "fakecc/compiler.h"
#include "fakecc/emit.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEP "\\"
#define TMP_DIR  "."
#else
#include <unistd.h>
#define PATH_SEP "/"
#define TMP_DIR  "/tmp"
#endif

static void test_options_init(void) {
    FakeccOptions opts;
    fakecc_options_init(&opts);
    T_ASSERT_EQ_INT(opts.opt_level, 1);
    T_ASSERT_EQ_INT(opts.want_debug, 0);
    T_ASSERT_EQ_INT(opts.nostdlib, 0);
}

static void test_compile_string_to_obj(void) {
    const char *src = "package main;\nint square(int x) { return x * x; }\n";
    const char *out = TMP_DIR PATH_SEP "test_compiler_sq.o";
    remove(out);

    int rc = fakecc_compile_string_to_obj(src, out, NULL);
    T_ASSERT_EQ_INT(rc, 0);

    EmitModule em;
    T_ASSERT_EQ_INT(emit_obj_read(out, &em), 0);
    T_ASSERT(em.text.len > 0);
    T_ASSERT(emit_module_find_symbol(&em, "square") >= 0);
    emit_module_free(&em);
    remove(out);
}

static void test_compile_string_to_so(void) {
    const char *src = "package main;\n"
                      "int add(int a, int b) { return a + b; }\n"
                      "int mul(int a, int b) { return a * b; }\n";
    const char *out = TMP_DIR PATH_SEP "test_compiler_lib.so";
    remove(out);

    FakeccOptions opts;
    fakecc_options_init(&opts);
    opts.opt_level = 1;

    int rc = fakecc_compile_string_to_so(src, out, &opts);
    T_ASSERT_EQ_INT(rc, 0);

    FILE *f = fopen(out, "rb");
    T_ASSERT(f != NULL);
    unsigned char magic[4];
    size_t n = fread(magic, 1, 4, f);
    fclose(f);
    T_ASSERT_EQ_INT(n, 4);
    /* ELF magic: 0x7f, 'E', 'L', 'F' */
    T_ASSERT_EQ_INT(magic[0], 0x7f);
    T_ASSERT_EQ_INT(magic[1], 'E');
    T_ASSERT_EQ_INT(magic[2], 'L');
    T_ASSERT_EQ_INT(magic[3], 'F');
    remove(out);
}

static void test_compile_file_to_so_and_executable(void) {
    const char *c_file = TMP_DIR PATH_SEP "test_compiler_tmp.c";
    const char *so_file = TMP_DIR PATH_SEP "test_compiler_tmp.so";
    const char *exe_file = TMP_DIR PATH_SEP "test_compiler_tmp_exe";

    FILE *f = fopen(c_file, "w");
    T_ASSERT(f != NULL);
    fputs("package main;\nint main(void) { return 42; }\n", f);
    fclose(f);

    /* Compile file to so */
    int rc1 = fakecc_compile_file_to_so(c_file, so_file, NULL);
    T_ASSERT_EQ_INT(rc1, 0);

    FILE *fso = fopen(so_file, "rb");
    T_ASSERT(fso != NULL);
    fclose(fso);

    /* Compile file to executable (using nostdlib or local runtime) */
    FakeccOptions opts;
    fakecc_options_init(&opts);
    opts.nostdlib = 1;
    int rc2 = fakecc_compile_file_to_executable(c_file, exe_file, &opts);
    T_ASSERT_EQ_INT(rc2, 0);

    FILE *fexe = fopen(exe_file, "rb");
    T_ASSERT(fexe != NULL);
    fclose(fexe);

    remove(c_file);
    remove(so_file);
    remove(exe_file);
}

/* Rejected inputs must release the TU (builtin typedefs included) and leave
 * the library usable.  A leak here is what OOM-killed fuzz_differential. */
static void test_reject_then_compile(void) {
    const char *out = TMP_DIR PATH_SEP "test_compiler_rej.o";
    int i;
    fakecc_set_quiet(1);
    for (i = 0; i < 40; i++) {
        int rc = fakecc_compile_string_to_obj("this is not c\n", out, NULL);
        T_ASSERT(rc != 0);
        rc = fakecc_compile_string_to_obj("package main;\n1 + 2;\n", out, NULL);
        T_ASSERT(rc != 0);
        rc = fakecc_compile_string_to_obj(
            "package main;\nint f(void) { return 0; }\nint f(void) { return 1; }\n", out, NULL);
        T_ASSERT(rc != 0);
        rc = fakecc_compile_string_to_obj(
            "package main;\nint f(int a, int a) { return a; }\n", out, NULL);
        T_ASSERT(rc != 0);
        rc = fakecc_compile_string_to_obj(
            "package main;\nint f(struct UndeclaredS s) { return 0; }\n", out, NULL);
        T_ASSERT(rc != 0);
    }
    fakecc_set_quiet(0);
    T_ASSERT_EQ_INT(
        fakecc_compile_string_to_obj(
            "package main;\nint main(void) { return 1; }\n", out, NULL),
        0);
    remove(out);
}

/* Bitfield expressions can create complex/nested types, reallocating
 * StructRegistry while parsing a struct body.  This must not use-after-free
 * or corrupt alignment/size. */
static void test_struct_bitfield_realloc(void) {
    const char *out = TMP_DIR PATH_SEP "test_compiler_bf_realloc.o";
    const char *src =
        "package main;\n"
        "struct s {\n"
        "    int a : (int)(sizeof(struct nested { int x; }) + 2);\n"
        "    int b : 4;\n"
        "} __attribute__((aligned(8)));\n"
        "int main(void) { return 0; }\n";
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(src, out, NULL), 0);
    remove(out);
}

/* A few hundred `+`s are a left spine.  The parser builds that iteratively,
 * so the cap is the input, not MAX_EXPR_DEPTH.  Freeing and type-checking it
 * must not crash. */
static void test_deep_add_chain(void) {
    const char *out = TMP_DIR PATH_SEP "test_compiler_deep.o";
    char *buf;
    size_t n = 0, cap = 8192;
    int i;
    buf = malloc(cap);
    T_ASSERT(buf != NULL);
    n += (size_t)sprintf(buf, "package main;\nint main(void) { return 1");
    for (i = 0; i < 400; i++) {
        if (n + 8 >= cap) {
            cap *= 2;
            buf = realloc(buf, cap);
            T_ASSERT(buf != NULL);
        }
        n += (size_t)sprintf(buf + n, "+1");
    }
    sprintf(buf + n, "; }\n");
    T_ASSERT_EQ_INT(fakecc_compile_string_to_obj(buf, out, NULL), 0);
    free(buf);

    /* Past the sema limit the compile must fail cleanly, not overflow. */
    cap = 64 + (size_t)600 * 2;
    buf = malloc(cap);
    T_ASSERT(buf != NULL);
    n = (size_t)sprintf(buf, "package main;\nint main(void) { return 1");
    for (i = 0; i < 600; i++)
        n += (size_t)sprintf(buf + n, "+1");
    sprintf(buf + n, "; }\n");
    fakecc_set_quiet(1);
    T_ASSERT(fakecc_compile_string_to_obj(buf, out, NULL) != 0);
    fakecc_set_quiet(0);
    free(buf);
    remove(out);
}

int main(void) {
    /* The high-level API cases below assert ELF output; pin the x86-64
     * backend until the arm64-macos backend lands. */
    target_set_current(target_x86_64_linux());
    test_options_init();
    test_compile_string_to_obj();
    test_compile_string_to_so();
    test_compile_file_to_so_and_executable();
    test_reject_then_compile();
    test_struct_bitfield_realloc();
    test_deep_add_chain();
    return t_finalize();
}
