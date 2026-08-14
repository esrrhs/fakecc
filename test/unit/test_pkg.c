#define _POSIX_C_SOURCE 200809L
#include "fakecc/pkg.h"
#include "test_framework.h"

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

int main(void) {
    test_load_and_export();
    test_cycle_detected();
    return t_finalize();
}
