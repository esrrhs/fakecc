#include "fakecc/compiler.h"
#include "fakecc/emit.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void test_options_init(void) {
    FakeccOptions opts;
    fakecc_options_init(&opts);
    T_ASSERT_EQ_INT(opts.opt_level, 1);
    T_ASSERT_EQ_INT(opts.want_debug, 0);
    T_ASSERT_EQ_INT(opts.nostdlib, 0);
}

static void test_compile_string_to_obj(void) {
    const char *src = "package main;\nint square(int x) { return x * x; }\n";
    const char *out = "/tmp/test_compiler_sq.o";
    unlink(out);

    int rc = fakecc_compile_string_to_obj(src, out, NULL);
    T_ASSERT_EQ_INT(rc, 0);

    EmitModule em;
    T_ASSERT_EQ_INT(emit_obj_read(out, &em), 0);
    T_ASSERT(em.text.len > 0);
    T_ASSERT(emit_module_find_symbol(&em, "square") >= 0);
    emit_module_free(&em);
    unlink(out);
}

static void test_compile_string_to_so(void) {
    const char *src = "package main;\n"
                      "int add(int a, int b) { return a + b; }\n"
                      "int mul(int a, int b) { return a * b; }\n";
    const char *out = "/tmp/test_compiler_lib.so";
    unlink(out);

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
    unlink(out);
}

static void test_compile_file_to_so_and_executable(void) {
    const char *c_file = "/tmp/test_compiler_tmp.c";
    const char *so_file = "/tmp/test_compiler_tmp.so";
    const char *exe_file = "/tmp/test_compiler_tmp_exe";

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

    unlink(c_file);
    unlink(so_file);
    unlink(exe_file);
}

int main(void) {
    /* The high-level API cases below assert ELF output; pin the x86-64
     * backend until the arm64-macos backend lands. */
    target_set_current(target_x86_64_linux());
    test_options_init();
    test_compile_string_to_obj();
    test_compile_string_to_so();
    test_compile_file_to_so_and_executable();
    return t_finalize();
}
