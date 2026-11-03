#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/target.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#else
#include "win_isolate.h"
#endif

/* ---- helper: compile source to temp executable ---- */
static void compile_to_file(const char *src, const char *out_path) {
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
    emit_elf(&em, out_path);

    emit_module_free(&em);
    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);
}

/* ---- tests ---- */

static void test_elf_magic(void) {
#ifdef _WIN32
    const char *out = win_tmp("fakecc_test_elf_magic");
#else
    const char *out = "/tmp/fakecc_test_elf_magic";
#endif
    compile_to_file("package main; int main() { return 0; }", out);

    FILE *f = fopen(out, "rb");
    T_ASSERT(f != NULL);
    unsigned char magic[4];
    T_ASSERT_EQ_INT((int)fread(magic, 1, 4, f), 4);
    fclose(f);
    T_ASSERT(magic[0] == 0x7f);
    T_ASSERT(magic[1] == 'E');
    T_ASSERT(magic[2] == 'L');
    T_ASSERT(magic[3] == 'F');
    unlink(out);
}

static void test_elf_machine(void) {
#ifdef _WIN32
    const char *out = win_tmp("fakecc_test_elf_machine");
#else
    const char *out = "/tmp/fakecc_test_elf_machine";
#endif
    compile_to_file("package main; int main() { return 0; }", out);

    FILE *f = fopen(out, "rb");
    T_ASSERT(f != NULL);
    fseek(f, 18, SEEK_SET); /* e_machine at offset 18 */
    uint16_t machine;
    T_ASSERT_EQ_INT((int)fread(&machine, 2, 1, f), 1);
    fclose(f);
    T_ASSERT_EQ_INT((int)machine, 62); /* EM_X86_64 */
    unlink(out);
}

/* The run-execution cases fork/exec a real x86-64 Linux process, so they
 * can only be exercised natively on an x86-64 Linux host.  Everywhere else
 * (Darwin arm64, Linux aarch64, ...) the artifacts are still produced --
 * the magic/machine checks cover codegen+emission -- but executing them
 * would just fail with ENOEXEC; those cases report SKIP instead. */
#if defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))
#define HOST_RUNS_X86_64_LINUX_ELF 1
#else
#define HOST_RUNS_X86_64_LINUX_ELF 0
#endif

/* Run an emitted executable and return its exit status (-1 if it did not
 * exit normally).  Assumes the caller already verified host support. */
#if HOST_RUNS_X86_64_LINUX_ELF
static int run_elf_exit_code(const char *out) {
    int pid = fork();
    if (pid == 0) {
        execl(out, out, (char *)NULL);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}
#endif

/* Compile on every host (that exercises the whole emit pipeline); only
 * execute where the kernel can load the artifact. */
#if HOST_RUNS_X86_64_LINUX_ELF
#define RUN_ELF_CASE(out, expect) \
    T_ASSERT_EQ_INT(run_elf_exit_code(out), (expect))
#else
#define RUN_ELF_CASE(out, expect) do {                                       \
    (void)(expect);                                                          \
    printf("  SKIP run-exec %s (x86-64 Linux ELF needs a Linux x86-64 "      \
           "host)\n", (out));                                                \
} while (0)
#endif

static void test_run_return_0(void) {
#ifdef _WIN32
    const char *out = win_tmp("fakecc_test_run_0");
#else
    const char *out = "/tmp/fakecc_test_run_0";
#endif
    compile_to_file("package main; int main() { return 0; }", out);

    RUN_ELF_CASE(out, 0);
    unlink(out);
}

static void test_run_return_42(void) {
#ifdef _WIN32
    const char *out = win_tmp("fakecc_test_run_42");
#else
    const char *out = "/tmp/fakecc_test_run_42";
#endif
    compile_to_file("package main; int main() { return 42; }", out);

    RUN_ELF_CASE(out, 42);
    unlink(out);
}

static void test_run_return_255(void) {
#ifdef _WIN32
    const char *out = win_tmp("fakecc_test_run_255");
#else
    const char *out = "/tmp/fakecc_test_run_255";
#endif
    compile_to_file("package main; int main() { return 255; }", out);

    RUN_ELF_CASE(out, 255);
    unlink(out);
}

/* ---- fakeos M4: ET_REL sections + reloc set ---- */

/* Compile to relocatable .o (same path as fakecc -c). */
static void compile_to_obj(const char *src, const char *out_path) {
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

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em, 0);
    emit_set_object_mode(1);
    emit_obj(&em, out_path);
    emit_set_object_mode(0);

    emit_module_free(&em);
    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);
}

/* Read whole file into malloc'd buffer; *n_out = size. */
static unsigned char *slurp(const char *path, size_t *n_out) {
    FILE *f = fopen(path, "rb");
    T_ASSERT(f != NULL);
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fseek(f, 0, SEEK_SET);
    T_ASSERT(n >= 64);
    unsigned char *b = (unsigned char *)malloc((size_t)n);
    T_ASSERT(b != NULL);
    T_ASSERT_EQ_INT((int)fread(b, 1, (size_t)n, f), (int)n);
    fclose(f);
    *n_out = (size_t)n;
    return b;
}

static uint16_t rd_u16(const unsigned char *p) {
    uint16_t v;
    memcpy(&v, p, 2);
    return v;
}
static uint32_t rd_u32(const unsigned char *p) {
    uint32_t v;
    memcpy(&v, p, 4);
    return v;
}
static uint64_t rd_u64(const unsigned char *p) {
    uint64_t v;
    memcpy(&v, p, 8);
    return v;
}

/* Return 1 if shstrtab contains the NUL-terminated name. */
static int elf_has_section_name(const unsigned char *b, size_t n, const char *name) {
    if (n < 64) return 0;
    uint64_t shoff = rd_u64(b + 40);
    uint16_t shentsize = rd_u16(b + 58);
    uint16_t shnum = rd_u16(b + 60);
    uint16_t shstrndx = rd_u16(b + 62);
    if (shentsize < 64 || shstrndx >= shnum) return 0;
    if (shoff + (uint64_t)shnum * shentsize > n) return 0;
    const unsigned char *shstr = b + shoff + (size_t)shstrndx * shentsize;
    uint64_t stroff = rd_u64(shstr + 24);
    uint64_t strsz = rd_u64(shstr + 32);
    if (stroff + strsz > n) return 0;
    size_t want = strlen(name) + 1;
    for (uint64_t i = 0; i + want <= strsz; i++) {
        if (memcmp(b + stroff + i, name, want) == 0) return 1;
    }
    return 0;
}

/* Scan all SHT_RELA sections for a relocation type. */
static int elf_has_rela_type(const unsigned char *b, size_t n, uint32_t want) {
    uint64_t shoff = rd_u64(b + 40);
    uint16_t shentsize = rd_u16(b + 58);
    uint16_t shnum = rd_u16(b + 60);
    for (uint16_t i = 0; i < shnum; i++) {
        const unsigned char *sh = b + shoff + (size_t)i * shentsize;
        uint32_t type = rd_u32(sh + 4);
        if (type != 4) continue; /* SHT_RELA */
        uint64_t off = rd_u64(sh + 24);
        uint64_t sz = rd_u64(sh + 32);
        uint64_t entsize = rd_u64(sh + 56);
        if (entsize < 24 || off + sz > n) continue;
        for (uint64_t o = 0; o + entsize <= sz; o += entsize) {
            uint64_t info = rd_u64(b + off + o + 8);
            uint32_t rtype = (uint32_t)(info & 0xffffffffu);
            if (rtype == want) return 1;
        }
    }
    return 0;
}

static void test_et_rel_standard_sections(void) {
    const char *out = "/tmp/fakecc_m4_sections.o";
    compile_to_obj(
        "package main;\n"
        "int g = 1;\n"
        "const int r = 2;\n"
        "int bss;\n"
        "int main(void) { return g + r + bss; }\n",
        out);
    size_t n = 0;
    unsigned char *b = slurp(out, &n);
    T_ASSERT_EQ_INT((int)rd_u16(b + 16), 1); /* ET_REL */
    T_ASSERT(elf_has_section_name(b, n, ".text"));
    T_ASSERT(elf_has_section_name(b, n, ".rodata"));
    T_ASSERT(elf_has_section_name(b, n, ".data"));
    T_ASSERT(elf_has_section_name(b, n, ".bss"));
    T_ASSERT(elf_has_section_name(b, n, ".rela.text")
             || elf_has_section_name(b, n, ".rela.data"));
    free(b);
    unlink(out);
}

static void test_et_rel_reloc_set_m4(void) {
    const char *out = "/tmp/fakecc_m4_relocs.o";
    /* Exercise PC32 / PLT32 / GOTPCREL / 64 (and typically 32S for locals). */
    compile_to_obj(
        "package main;\n"
        "extern void ext_fn(void);\n"
        "extern int ext_var;\n"
        "int g = 1;\n"
        "int *gp = &g;\n"
        "void f(void) {\n"
        "    g = ext_var;\n"
        "    ext_fn();\n"
        "}\n"
        "int main(void) { f(); return *gp; }\n",
        out);
    size_t n = 0;
    unsigned char *b = slurp(out, &n);
    T_ASSERT_EQ_INT((int)rd_u16(b + 16), 1); /* ET_REL */
    T_ASSERT(elf_has_rela_type(b, n, R_X86_64_64));
    T_ASSERT(elf_has_rela_type(b, n, R_X86_64_PC32));
    T_ASSERT(elf_has_rela_type(b, n, R_X86_64_PLT32));
    T_ASSERT(elf_has_rela_type(b, n, R_X86_64_GOTPCREL));
    free(b);
    unlink(out);

    /* Round-trip R_X86_64_32 (nasm / stub style) through emit_obj. */
    EmitModule em;
    emit_module_init(&em);
    /* one nop so .text is non-empty */
    {
        char nop = (char)0x90;
        buffer_append(&em.text, &nop, 1);
    }
    int sym = emit_module_add_undefined(&em, "stub_sym");
    emit_module_add_reloc(&em, 0, R_X86_64_32, sym, 0);
    emit_module_add_reloc(&em, 0, R_X86_64_32S, sym, 0);
    const char *out2 = "/tmp/fakecc_m4_reloc32.o";
    emit_set_object_mode(1);
    emit_obj(&em, out2);
    emit_set_object_mode(0);
    emit_module_free(&em);

    b = slurp(out2, &n);
    T_ASSERT(elf_has_rela_type(b, n, R_X86_64_32));
    T_ASSERT(elf_has_rela_type(b, n, R_X86_64_32S));
    free(b);
    unlink(out2);
}

static void test_mcmodel_kernel_flag(void) {
    codegen_set_code_model(0);
    T_ASSERT(!codegen_code_model_is_kernel());
    codegen_set_code_model(1);
    T_ASSERT(codegen_code_model_is_kernel());
    codegen_set_code_model(0);
}

/* ---- main ---- */

int main(void) {
    /* Pins the x86-64 ELF backend; the run-execution cases execute only on
     * a Linux x86-64 host and report SKIP elsewhere. */
    target_set_current(target_x86_64_linux());
    test_elf_magic();
    test_elf_machine();
    test_et_rel_standard_sections();
    test_et_rel_reloc_set_m4();
    test_mcmodel_kernel_flag();
    test_run_return_0();
    test_run_return_42();
    test_run_return_255();
    return t_finalize();
}
