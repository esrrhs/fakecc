#define _POSIX_C_SOURCE 200809L
#include "fakecc/ast.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/pkg.h"
#include "fakecc/sema.h"
#include "test_framework.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

static char *write_file(const char *dir, const char *name, const char *body) {
    size_t n = strlen(dir) + 1 + strlen(name) + 1;
    char *path = malloc(n);
    snprintf(path, n, "%s/%s", dir, name);
    FILE *f = fopen(path, "w");
    T_ASSERT(f != NULL);
    fputs(body, f);
    fclose(f);
    return path;
}

static void test_load_and_export(void) {
    char tmpl[] = "/tmp/fakecc_pkg_XXXXXX";
    char *root = mkdtemp(tmpl);
    T_ASSERT(root != NULL);

    char pkgdir[256];
    snprintf(pkgdir, sizeof pkgdir, "%s/util", root);
    T_ASSERT(mkdir(pkgdir, 0755) == 0);

    char *p1 = write_file(pkgdir, "a.c",
        "package util;\n"
        "int add(int x, int y) { return x + y; }\n"
        "static int hidden(void) { return 0; }\n");
    char *p2 = write_file(pkgdir, "b.c",
        "package util;\n"
        "int add(int x, int y);\n"
        "int double_it(int x) { return add(x, x); }\n");

    PkgContext ctx;
    pkg_ctx_init(&ctx);
    pkg_ctx_add_path(&ctx, root);

    SourceLoc loc = {0};
    Package *pkg = pkg_load(&ctx, "util", loc);
    T_ASSERT(pkg != NULL);
    T_ASSERT(strcmp(pkg->name, "util") == 0);
    T_ASSERT(pkg->nfiles == 2);
    T_ASSERT(pkg_find_func(pkg, "add") != NULL);
    T_ASSERT(pkg_find_func(pkg, "double_it") != NULL);
    T_ASSERT(pkg_find_func(pkg, "hidden") == NULL); /* static */
    T_ASSERT(pkg_load(&ctx, "util", loc) == pkg); /* cache hit */

    pkg_ctx_free(&ctx);
    free(p1);
    free(p2);
}

static void test_cycle_detected(void) {
    char tmpl[] = "/tmp/fakecc_pkg_XXXXXX";
    char *root = mkdtemp(tmpl);
    T_ASSERT(root != NULL);

    char pa[256], pb[256];
    snprintf(pa, sizeof pa, "%s/pa", root);
    snprintf(pb, sizeof pb, "%s/pb", root);
    T_ASSERT(mkdir(pa, 0755) == 0);
    T_ASSERT(mkdir(pb, 0755) == 0);
    free(write_file(pa, "a.c",
                    "package pa;\nimport pb;\nint a(void) { return 0; }\n"));
    free(write_file(pb, "b.c",
                    "package pb;\nimport pa;\nint b(void) { return 0; }\n"));

    int pid = fork();
    if (pid == 0) {
        PkgContext ctx;
        pkg_ctx_init(&ctx);
        pkg_ctx_add_path(&ctx, root);
        SourceLoc loc = {0};
        pkg_load(&ctx, "pa", loc); /* should die on cycle */
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

static int child_dies(void (*fn)(void)) {
    int pid = fork();
    if (pid == 0) {
        int nulfd = open("/dev/null", O_WRONLY);
        if (nulfd >= 0) {
            dup2(nulfd, STDERR_FILENO);
            close(nulfd);
        }
        fn();
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) && WEXITSTATUS(status) != 0;
}

static char g_root[256];
static const char *g_load_name;
static const char *g_src;
static const char *g_preload;

static void do_pkg_load(void) {
    PkgContext ctx;
    pkg_ctx_init(&ctx);
    pkg_ctx_add_path(&ctx, g_root);
    SourceLoc loc = {0};
    pkg_load(&ctx, g_load_name, loc);
    pkg_ctx_free(&ctx);
}

static void do_parse_sema(void) {
    PkgContext ctx;
    pkg_ctx_init(&ctx);
    pkg_ctx_add_path(&ctx, g_root);
    SourceLoc loc = {0};
    if (g_preload)
        pkg_load(&ctx, g_preload, loc);
    TokenArray arr;
    token_array_init(&arr);
    lex(g_src, "t.c", &arr);
    TranslationUnit tu;
    tu_init(&tu);
    parse_in_pkg(&arr, &tu, &ctx);
    token_array_free(&arr);
    sema_check_in_pkg(&tu, 1, &ctx);
    tu_free(&tu);
    pkg_ctx_free(&ctx);
}

static char *make_root(void) {
    char tmpl[] = "/tmp/fakecc_pkg_XXXXXX";
    char *root = mkdtemp(tmpl);
    T_ASSERT(root != NULL);
    snprintf(g_root, sizeof g_root, "%s", root);
    return g_root;
}

static void mkdir_pkg(const char *root, const char *name) {
    char dir[256];
    snprintf(dir, sizeof dir, "%s/%s", root, name);
    T_ASSERT(mkdir(dir, 0755) == 0);
}

static void test_pkg_die_at(void) {
    char *root = make_root();
    mkdir_pkg(root, "empty");
    g_load_name = "empty";
    T_ASSERT(child_dies(do_pkg_load)); /* no .c files → package not found */

    g_load_name = "missing";
    T_ASSERT(child_dies(do_pkg_load));

    mkdir_pkg(root, "wrong");
    free(write_file(root, "wrong/a.c", "package other;\nint x;\n"));
    g_load_name = "wrong";
    T_ASSERT(child_dies(do_pkg_load));

    mkdir_pkg(root, "util");
    free(write_file(root, "util/a.c",
                    "package util;\n"
                    "int add(int x, int y) { return x + y; }\n"));
    mkdir_pkg(root, "shapes");
    free(write_file(root, "shapes/s.c",
                    "package shapes;\n"
                    "typedef struct S { int x; int y; } T;\n"));

    g_preload = NULL;
    g_src = "package main; import util; import util; int main(){return 0;}";
    T_ASSERT(child_dies(do_parse_sema));

    g_src = "package main; import util; util.NoSuch x; int main(){return 0;}";
    T_ASSERT(child_dies(do_parse_sema));

    g_src = "package main; import util; int main() { return util.nope; }";
    T_ASSERT(child_dies(do_parse_sema));

    g_src = "package main; import util; int main() { return add(1, 2); }";
    T_ASSERT(child_dies(do_parse_sema));

    g_preload = "util";
    g_src = "package main; int main() { return add(1, 2); }";
    T_ASSERT(child_dies(do_parse_sema));

    g_preload = NULL;
    g_src = "package main; import shapes; struct S { int x; }; shapes.T v; "
            "int main(){return 0;}";
    T_ASSERT(child_dies(do_parse_sema));
}

static void test_pkg_typedef_enum(void) {
    char tmpl[] = "/tmp/fakecc_pkg_XXXXXX";
    char *root = mkdtemp(tmpl);
    T_ASSERT(root != NULL);

    char pkgdir[256];
    snprintf(pkgdir, sizeof pkgdir, "%s/util", root);
    T_ASSERT(mkdir(pkgdir, 0755) == 0);

    char *p1 = write_file(pkgdir, "a.c",
        "package util;\n"
        "struct Node { int v; };\n"
        "typedef struct Node NodeT;\n"
        "typedef struct Node *NodeP;\n"
        "enum Color { RED = 1, BLUE = 2 };\n");
    char *p2 = write_file(pkgdir, "z.c",
        "package util;\n"
        "NodeP gp;\n"
        "NodeT gt;\n"
        "int n = RED;\n");

    PkgContext ctx;
    pkg_ctx_init(&ctx);
    pkg_ctx_add_path(&ctx, root);

    SourceLoc loc = {0};
    Package *pkg = pkg_load(&ctx, "util", loc);
    T_ASSERT(pkg != NULL);
    T_ASSERT(pkg_find_enum(pkg, "Color") != NULL);
    T_ASSERT(pkg_find_typedef(pkg, "NodeT") != NULL);
    T_ASSERT(pkg_find_typedef(pkg, "NodeP") != NULL);

    TranslationUnit tu;
    tu_init(&tu);
    typedef_registry_add(&tu.typedefs, "NodeT", type_make_int(4, 0));
    pkg_import_typedef(&tu, "NodeT", pkg_find_typedef(pkg, "NodeT"), pkg);
    pkg_import_typedef(&tu, "NodeP", pkg_find_typedef(pkg, "NodeP"), pkg);
    T_ASSERT(typedef_registry_find(&tu.typedefs, "NodeP") != NULL);
    tu_free(&tu);

    pkg_ctx_free(&ctx);
    free(p1);
    free(p2);
}

int main(void) {
    test_load_and_export();
    test_cycle_detected();
    test_pkg_die_at();
    test_pkg_typedef_enum();
    return t_finalize();
}
