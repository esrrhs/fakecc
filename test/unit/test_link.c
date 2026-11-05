#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
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

/* Compile one TU into an EmitModule (no main required). */
static void compile_tu(const char *src, EmitModule *out) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu, 0);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir, 0);
    opt(&ir, 1, 0);

    emit_module_init(out);
    /* Mach-O links relocatable modules.  The ELF path still emits the
     * in-memory image codegen has always produced. */
    int as_obj = target_current()->objfmt == TARGET_OBJFMT_MACHO;
    if (as_obj) emit_set_object_mode(1);
    codegen(&ir, out, 0);
    if (as_obj) emit_set_object_mode(0);

    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&arr);
}

static int fork_dies(void (*fn)(void)) {
    int pid = fork();
    if (pid == 0) {
        fn();
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) != 0;
}

static void test_obj_roundtrip(void) {
#ifdef _WIN32
    const char *path = win_tmp("fakecc_test_link_roundtrip.o");
#else
    const char *path = "/tmp/fakecc_test_link_roundtrip.o";
#endif
    EmitModule em;
    compile_tu("package main; int add(int a, int b) { return a + b; }", &em);
    emit_obj(&em, path);
    emit_module_free(&em);

    EmitModule got;
    T_ASSERT_EQ_INT(emit_obj_read(path, &got), 0);
    T_ASSERT(got.text.len > 0);
    T_ASSERT(emit_module_find_symbol(&got, "add") >= 0);
    emit_module_free(&got);
    unlink(path);
}

static void test_link_two_modules(void) {
#ifdef _WIN32
    const char *path = win_tmp("fakecc_test_link_two.exe");
#else
    const char *path = "/tmp/fakecc_test_link_two";
#endif
    EmitModule a, b;
    compile_tu("package main; int add(int a, int b) { return a + b; }", &a);
    compile_tu("package main; int add(int a, int b); int main(void) { return add(20, 22); }", &b);

    EmitModule *mods[2];
    mods[0] = &a;
    mods[1] = &b;
    emit_link(mods, 2, path, NULL, 0, 0, NULL, 0, 0, 0);
    emit_module_free(&a);
    emit_module_free(&b);

    int pid = fork();
    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    int got = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    T_ASSERT_EQ_INT(got, 42);
    unlink(path);
}

static void link_no_main_body(void) {
    EmitModule a;
    compile_tu("package main; int foo(void) { return 1; }", &a);
    EmitModule *mods[1];
    mods[0] = &a;
#ifdef _WIN32
    emit_link(mods, 1, win_tmp("fakecc_test_link_nomain.exe"), NULL, 0, 0, NULL, 0, 0, 0);
#else
    emit_link(mods, 1, "/tmp/fakecc_test_link_nomain", NULL, 0, 0, NULL, 0, 0, 0);
#endif
    emit_module_free(&a);
}

static void test_link_rejects_no_main(void) {
    T_ASSERT(fork_dies(link_no_main_body));
}

static void test_link_static_no_collision(void) {
#ifdef _WIN32
    const char *path = win_tmp("fakecc_test_link_static.exe");
#else
    const char *path = "/tmp/fakecc_test_link_static";
#endif
    EmitModule a, b;
    compile_tu("package main; static int x = 9; int get(void) { return x; }", &a);
    compile_tu("package main; static int x = 1; int get(void); int main(void) { return get() + x; }", &b);

    EmitModule *mods[2];
    mods[0] = &a;
    mods[1] = &b;
    emit_link(mods, 2, path, NULL, 0, 0, NULL, 0, 0, 0);
    emit_module_free(&a);
    emit_module_free(&b);

    int pid = fork();
    if (pid == 0) {
        execl(path, path, (char *)NULL);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    int got = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    T_ASSERT_EQ_INT(got, 10);
    unlink(path);
}


/* fakeos S1: default image base is USER_VMA; --image-base overrides. */
static void test_image_base_user_vma(void) {
    T_ASSERT_EQ_INT((int)(link_image_base() == 0x400000ULL), 1);
    link_set_image_base(0x800000ULL);
    T_ASSERT_EQ_INT((int)(link_image_base() == 0x800000ULL), 1);
    /* restore default for later cases */
    link_set_image_base(0);
    T_ASSERT_EQ_INT((int)(link_image_base() == 0x400000ULL), 1);

#if defined(__linux__) && (defined(__x86_64__) || defined(__amd64__))
    const char *path = "/tmp/fakecc_test_link_image_base";
    EmitModule em;
    compile_tu("package main; int main(void) { return 7; }", &em);
    EmitModule *mods[1] = { &em };
    link_set_image_base(0x600000ULL);
    emit_link(mods, 1, path, NULL, 0, 0, NULL, 0, 0, 0);
    link_set_image_base(0);
    emit_module_free(&em);

    FILE *f = fopen(path, "rb");
    T_ASSERT(f != NULL);
    unsigned char ehdr[64];
    T_ASSERT_EQ_INT((int)fread(ehdr, 1, 64, f), 64);
    uint64_t phoff;
    memcpy(&phoff, ehdr + 32, 8);
    uint16_t phentsize, phnum;
    memcpy(&phentsize, ehdr + 54, 2);
    memcpy(&phnum, ehdr + 56, 2);
    T_ASSERT(phnum >= 1);
    T_ASSERT(fseek(f, (long)phoff, SEEK_SET) == 0);
    unsigned char *phdr = (unsigned char *)malloc(phentsize);
    T_ASSERT(phdr != NULL);
    T_ASSERT_EQ_INT((int)fread(phdr, 1, phentsize, f), (int)phentsize);
    uint32_t p_type;
    memcpy(&p_type, phdr, 4);
    T_ASSERT_EQ_INT((int)p_type, 1); /* PT_LOAD */
    uint64_t p_vaddr;
    memcpy(&p_vaddr, phdr + 16, 8);
    /* First LOAD maps the file from image base (headers at base). */
    T_ASSERT_EQ_INT((int)(p_vaddr == 0x600000ULL), 1);
    free(phdr);
    fclose(f);
    unlink(path);
#endif
}

int main(void) {
    /* On this Mac the scenarios run as arm64 Mach-O.  Elsewhere they stay
     * pinned to the x86-64 ELF backend, which executes natively on Linux. */
#if defined(__APPLE__) && defined(__aarch64__)
    target_set_current(target_arm64_macos());
#elif defined(_WIN32)
    target_set_current(target_x86_64_windows());
#else
    target_set_current(target_x86_64_linux());
#endif
    test_image_base_user_vma();
    test_obj_roundtrip();
    test_link_two_modules();
    test_link_rejects_no_main();
    test_link_static_no_collision();
    return t_finalize();
}
