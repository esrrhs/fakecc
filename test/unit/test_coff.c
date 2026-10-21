/* Unit tests for AMD64 COFF object write/read and MinGW interoperability. */
#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/pe.h"
#include "fakecc/sema.h"
#include "fakecc/target.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _WIN32
#define PATH_SEP "\\"
#define TMP_DIR  "."
#else
#define PATH_SEP "/"
#define TMP_DIR  "/tmp"
#endif

static void compile_tu(const char *src, EmitModule *out) {
    target_set_current(target_x86_64_windows());
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "t.c", &arr);
    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 0);
    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 0);
    opt(&ir, 1, 0);
    emit_module_init(out);
    emit_set_object_mode(1);
    codegen(&ir, out, 0);
    emit_set_object_mode(0);
    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&arr);
}

static const EmitSymbol *find_sym(const EmitModule *m, const char *name) {
    for (size_t i = 0; i < m->num_syms; i++)
        if (m->syms[i].name && strcmp(m->syms[i].name, name) == 0)
            return &m->syms[i];
    return NULL;
}

static void test_coff_roundtrip(void) {
    const char *path = TMP_DIR PATH_SEP "fakecc_test_coff_rt.obj";
    EmitModule em;
    compile_tu(
        "package main;\n"
        "static int local_x = 7;\n"
        "int add(int a, int b) { return a + b + local_x; }\n"
        "extern int missing;\n"
        "int *p = &missing;\n",
        &em);
    T_ASSERT(em.text.len > 0);
    T_ASSERT(em.num_win_unwinds > 0);
    T_ASSERT_EQ_INT(coff_write_object(&em, path), 0);
    emit_module_free(&em);

    EmitModule got;
    T_ASSERT_EQ_INT(coff_read_object(path, &got), 0);
    T_ASSERT(got.text.len > 0);
    const EmitSymbol *add = find_sym(&got, "add");
    T_ASSERT(add != NULL);
    T_ASSERT_EQ_INT((int)add->binding, 1); /* STB_GLOBAL */
    T_ASSERT_EQ_INT((int)add->type, 2);    /* STT_FUNC */
    T_ASSERT_EQ_INT((int)add->shndx, SECT_TEXT);
    const EmitSymbol *loc = find_sym(&got, "local_x");
    T_ASSERT(loc != NULL);
    T_ASSERT_EQ_INT((int)loc->binding, 0); /* STB_LOCAL */
    const EmitSymbol *miss = find_sym(&got, "missing");
    T_ASSERT(miss != NULL);
    T_ASSERT_EQ_INT((int)miss->shndx, SECT_UNDEF);
    /* pdata/xdata emitted for fakecc functions with win_unwinds */
    T_ASSERT(got.pdata.len >= 12);
    T_ASSERT(got.xdata.len > 0);
    int saw_pc = 0, saw_abs = 0;
    for (size_t i = 0; i < got.num_relocs; i++) {
        if (got.relocs[i].type == R_X86_64_PC32
            || got.relocs[i].type == R_X86_64_PLT32)
            saw_pc = 1;
    }
    for (size_t i = 0; i < got.num_data_relocs; i++) {
        if (got.data_relocs[i].type == R_X86_64_64) saw_abs = 1;
    }
    T_ASSERT(saw_pc || got.num_relocs == 0);
    T_ASSERT(saw_abs);
    emit_module_free(&got);
    remove(path);
}

static void test_coff_long_name(void) {
    const char *path = TMP_DIR PATH_SEP "fakecc_test_coff_long.obj";
    EmitModule em;
    compile_tu(
        "package main;\n"
        "int this_is_a_very_long_function_name_for_coff_string_table(void) {\n"
        "  return 42;\n"
        "}\n",
        &em);
    T_ASSERT_EQ_INT(coff_write_object(&em, path), 0);
    emit_module_free(&em);
    EmitModule got;
    T_ASSERT_EQ_INT(coff_read_object(path, &got), 0);
    T_ASSERT(find_sym(&got, "this_is_a_very_long_function_name_for_coff_string_table")
             != NULL);
    emit_module_free(&got);
    remove(path);
}

static void test_coff_emit_obj_dispatch(void) {
    const char *path = TMP_DIR PATH_SEP "fakecc_test_coff_dispatch.obj";
    target_set_current(target_x86_64_windows());
    EmitModule em;
    compile_tu("package main; int f(void) { return 1; }\n", &em);
    emit_obj(&em, path);
    emit_module_free(&em);
    EmitModule got;
    T_ASSERT_EQ_INT(emit_obj_read(path, &got), 0);
    T_ASSERT(find_sym(&got, "f") != NULL);
    emit_module_free(&got);
    remove(path);
}

static void test_coff_link_two_objs(void) {
    const char *a = TMP_DIR PATH_SEP "fakecc_test_coff_a.obj";
    const char *b = TMP_DIR PATH_SEP "fakecc_test_coff_b.obj";
    const char *exe = TMP_DIR PATH_SEP "fakecc_test_coff_ab.exe";
    EmitModule ma, mb;
    compile_tu("package main; int add(int x, int y) { return x + y; }\n", &ma);
    compile_tu(
        "package main; int add(int x, int y); int main(void) { return add(20, 22); }\n",
        &mb);
    T_ASSERT_EQ_INT(coff_write_object(&ma, a), 0);
    T_ASSERT_EQ_INT(coff_write_object(&mb, b), 0);
    emit_module_free(&ma);
    emit_module_free(&mb);

    EmitModule ra, rb;
    T_ASSERT_EQ_INT(coff_read_object(a, &ra), 0);
    T_ASSERT_EQ_INT(coff_read_object(b, &rb), 0);
    EmitModule *mods[2] = { &ra, &rb };
    T_ASSERT_EQ_INT(pe_link(mods, 2, exe), 0);
    emit_module_free(&ra);
    emit_module_free(&rb);
    remove(a);
    remove(b);
#ifdef _WIN32
    int code = system(exe);
    /* system() returns the process exit code shifted on some CRTs; accept 42
     * or the encoded form. */
    T_ASSERT(code == 42 || code == (42 << 8));
#endif
    remove(exe);
}

/* MinGW interoperability: compile a tiny C file with gcc -c and read it. */
static void test_coff_mingw_read(void) {
    const char *src = TMP_DIR PATH_SEP "fakecc_mingw_probe.c";
    const char *obj = TMP_DIR PATH_SEP "fakecc_mingw_probe.o";
    FILE *f = fopen(src, "w");
    T_ASSERT(f != NULL);
    fputs("int mingw_add(int a, int b) { return a + b; }\n"
          "int mingw_g = 3;\n"
          "static int mingw_local = 9;\n"
          "int *mingw_p = &mingw_g;\n", f);
    fclose(f);
    char cmd[512];
#ifdef _WIN32
    snprintf(cmd, sizeof cmd, "gcc -c -O0 -o \"%s\" \"%s\" 2>nul", obj, src);
#else
    snprintf(cmd, sizeof cmd, "gcc -c -O0 -o \"%s\" \"%s\" 2>/dev/null", obj, src);
#endif
    int rc = system(cmd);
    if (rc != 0) {
        /* MinGW/gcc unavailable — skip without failing the suite. */
        remove(src);
        return;
    }
    /* Linux and macOS gcc write ELF or Mach-O, not AMD64 COFF. */
    {
        FILE *of = fopen(obj, "rb");
        unsigned char mag[2] = {0, 0};
        size_t nread = of ? fread(mag, 1, 2, of) : 0;
        if (of) fclose(of);
        if (nread != 2 || mag[0] != 0x64 || mag[1] != 0x86) {
            remove(src);
            remove(obj);
            return;
        }
    }
    target_set_current(target_x86_64_windows());
    EmitModule m;
    T_ASSERT_EQ_INT(coff_read_object(obj, &m), 0);
    T_ASSERT(find_sym(&m, "mingw_add") != NULL);
    T_ASSERT(find_sym(&m, "mingw_g") != NULL);
    T_ASSERT(m.text.len > 0);
    /* MinGW often emits COMDAT for strings / sections; metadata may be empty
     * for this simple TU — just ensure read succeeded. */
    emit_module_free(&m);

    /* Also via emit_obj_read dispatch. */
    EmitModule m2;
    T_ASSERT_EQ_INT(emit_obj_read(obj, &m2), 0);
    T_ASSERT(find_sym(&m2, "mingw_add") != NULL);
    emit_module_free(&m2);
    remove(src);
    remove(obj);
}

static void test_coff_drectve_parse(void) {
    /* Synthesize a minimal COFF with .drectve via write of an empty module
     * that carries a directive buffer. */
    const char *path = TMP_DIR PATH_SEP "fakecc_test_drectve.obj";
    target_set_current(target_x86_64_windows());
    EmitModule em;
    emit_module_init(&em);
    buffer_append(&em.text, "\xC3", 1); /* ret */
    emit_module_add_symbol(&em, "f", 1, 2, SECT_TEXT, 0, 1);
    const char *dir = " -defaultlib:uuid ";
    buffer_append(&em.drectve, dir, strlen(dir));
    T_ASSERT_EQ_INT(coff_write_object(&em, path), 0);
    emit_module_free(&em);
    EmitModule got;
    T_ASSERT_EQ_INT(coff_read_object(path, &got), 0);
    T_ASSERT(got.drectve.len >= strlen(dir));
    T_ASSERT(memcmp(got.drectve.data, dir, strlen(dir)) == 0);
    emit_module_free(&got);
    remove(path);
}

static void test_coff_tls_roundtrip(void) {
    const char *path = TMP_DIR PATH_SEP "fakecc_test_coff_tls.obj";
    EmitModule em;
    compile_tu(
        "package main;\n"
        "__thread int g_tls = 42;\n"
        "__thread int g_tbss;\n"
        "int get_tls(void) {\n"
        "    g_tbss = g_tls + 1;\n"
        "    return g_tbss;\n"
        "}\n",
        &em);

    T_ASSERT(em.tdata.len > 0);
    T_ASSERT(em.tbss_size > 0);
    T_ASSERT_EQ_INT(coff_write_object(&em, path), 0);
    emit_module_free(&em);

    EmitModule got;
    T_ASSERT_EQ_INT(coff_read_object(path, &got), 0);
    T_ASSERT(find_sym(&got, "g_tls") != NULL);
    T_ASSERT(find_sym(&got, "g_tbss") != NULL);
    T_ASSERT(find_sym(&got, "get_tls") != NULL);
    T_ASSERT(got.tdata.len > 0);
    T_ASSERT(got.tbss_size > 0);

    int has_secrel = 0;
    for (size_t i = 0; i < got.num_relocs; i++) {
        if (got.relocs[i].type == R_PE_SECREL) has_secrel = 1;
    }
    T_ASSERT(has_secrel);

    emit_module_free(&got);
    remove(path);
}

static void test_coff_tls_aligned_pointer(void) {
    const char *path = TMP_DIR PATH_SEP "fakecc_test_coff_tls_align.obj";
    EmitModule em;
    compile_tu(
        "package main;\n"
        "int target = 42;\n"
        "__thread int *p_target = &target;\n"
        "__thread int tx __attribute__((aligned(64))) = 99;\n"
        "__thread int tbss_arr[16];\n",
        &em);

    T_ASSERT(em.tdata.len > 0);
    T_ASSERT(em.tbss_size >= 64);
    T_ASSERT(em.tdata_align >= 64);

    int has_dir64 = 0;
    for (size_t i = 0; i < em.num_data_relocs; i++) {
        if (em.data_relocs[i].shndx == SECT_TDATA &&
            em.data_relocs[i].type == R_X86_64_64) {
            has_dir64 = 1;
        }
    }
    T_ASSERT(has_dir64);

    T_ASSERT_EQ_INT(coff_write_object(&em, path), 0);
    emit_module_free(&em);

    EmitModule got;
    T_ASSERT_EQ_INT(coff_read_object(path, &got), 0);
    T_ASSERT(find_sym(&got, "p_target") != NULL);
    T_ASSERT(find_sym(&got, "tx") != NULL);
    T_ASSERT(find_sym(&got, "tbss_arr") != NULL);
    T_ASSERT(got.tdata.len > 0);
    T_ASSERT(got.tbss_size >= 64);
    T_ASSERT(got.tdata_align >= 64);

    int got_dir64 = 0;
    for (size_t i = 0; i < got.num_data_relocs; i++) {
        if (got.data_relocs[i].shndx == SECT_TDATA &&
            got.data_relocs[i].type == R_X86_64_64) {
            got_dir64 = 1;
        }
    }
    T_ASSERT(got_dir64);

    emit_module_free(&got);
    remove(path);
}

int main(void) {
    target_set_current(target_x86_64_windows());
    test_coff_roundtrip();
    test_coff_long_name();
    test_coff_emit_obj_dispatch();
    test_coff_link_two_objs();
    test_coff_mingw_read();
    test_coff_drectve_parse();
    test_coff_tls_roundtrip();
    test_coff_tls_aligned_pointer();
    return t_finalize();
}
