#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ---- helper: compile source to temp executable ---- */
static void compile_to_file(const char *src, const char *out_path) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&arr, &tu);
    sema_check(&tu);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir);

    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em);
    emit_elf(&em, out_path);

    emit_module_free(&em);
    ir_module_free(&ir);
    token_array_free(&arr);
    tu_free(&tu);
}

/* ---- tests ---- */

static void test_elf_magic(void) {
    const char *out = "/tmp/fakecc_test_elf_magic";
    compile_to_file("package main; int main() { return 0; }", out);

    FILE *f = fopen(out, "rb");
    T_ASSERT(f != NULL);
    unsigned char magic[4];
    fread(magic, 1, 4, f);
    fclose(f);
    T_ASSERT(magic[0] == 0x7f);
    T_ASSERT(magic[1] == 'E');
    T_ASSERT(magic[2] == 'L');
    T_ASSERT(magic[3] == 'F');
    unlink(out);
}

static void test_elf_machine(void) {
    const char *out = "/tmp/fakecc_test_elf_machine";
    compile_to_file("package main; int main() { return 0; }", out);

    FILE *f = fopen(out, "rb");
    T_ASSERT(f != NULL);
    fseek(f, 18, SEEK_SET); /* e_machine at offset 18 */
    uint16_t machine;
    fread(&machine, 2, 1, f);
    fclose(f);
    T_ASSERT_EQ_INT((int)machine, 62); /* EM_X86_64 */
    unlink(out);
}

static void test_run_return_0(void) {
    const char *out = "/tmp/fakecc_test_run_0";
    compile_to_file("package main; int main() { return 0; }", out);

    int pid = fork();
    if (pid == 0) {
        execl(out, out, (char *)NULL);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    int got = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    T_ASSERT_EQ_INT(got, 0);
    unlink(out);
}

static void test_run_return_42(void) {
    const char *out = "/tmp/fakecc_test_run_42";
    compile_to_file("package main; int main() { return 42; }", out);

    int pid = fork();
    if (pid == 0) {
        execl(out, out, (char *)NULL);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    int got = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    T_ASSERT_EQ_INT(got, 42);
    unlink(out);
}

static void test_run_return_255(void) {
    const char *out = "/tmp/fakecc_test_run_255";
    compile_to_file("package main; int main() { return 255; }", out);

    int pid = fork();
    if (pid == 0) {
        execl(out, out, (char *)NULL);
        _exit(127);
    }
    int status;
    waitpid(pid, &status, 0);
    int got = WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    T_ASSERT_EQ_INT(got, 255);
    unlink(out);
}

/* ---- main ---- */

int main(void) {
    test_elf_magic();
    test_elf_machine();
    test_run_return_0();
    test_run_return_42();
    test_run_return_255();
    return t_finalize();
}
