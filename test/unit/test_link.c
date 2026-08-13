#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

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
    ir_generate(&tu, &ir);
    opt(&ir);

    emit_module_init(out);
    codegen(&ir, out);

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
    const char *path = "/tmp/fakecc_test_link_roundtrip.o";
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
    const char *path = "/tmp/fakecc_test_link_two";
    EmitModule a, b;
    compile_tu("package main; int add(int a, int b) { return a + b; }", &a);
    compile_tu("package main; int add(int a, int b); int main(void) { return add(20, 22); }", &b);

    EmitModule *mods[2];
    mods[0] = &a;
    mods[1] = &b;
    emit_link(mods, 2, path);
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
    emit_link(mods, 1, "/tmp/fakecc_test_link_nomain");
    emit_module_free(&a);
}

static void test_link_rejects_no_main(void) {
    T_ASSERT(fork_dies(link_no_main_body));
}

static void test_link_static_no_collision(void) {
    const char *path = "/tmp/fakecc_test_link_static";
    EmitModule a, b;
    compile_tu("package main; static int x = 9; int get(void) { return x; }", &a);
    compile_tu("package main; static int x = 1; int get(void); int main(void) { return get() + x; }", &b);

    EmitModule *mods[2];
    mods[0] = &a;
    mods[1] = &b;
    emit_link(mods, 2, path);
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

int main(void) {
    test_obj_roundtrip();
    test_link_two_modules();
    test_link_rejects_no_main();
    test_link_static_no_collision();
    return t_finalize();
}
