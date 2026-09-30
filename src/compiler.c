#include "fakecc/compiler.h"
#include "fakecc/ast.h"
#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/pkg.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int file_readable(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fclose(f);
    return 1;
}

static char *path_join(const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    int slash = (na > 0 && a[na - 1] != '/');
    char *p = malloc(na + (size_t)slash + nb + 1);
    if (!p) return NULL;
    memcpy(p, a, na);
    if (slash) p[na++] = '/';
    memcpy(p + na, b, nb + 1);
    return p;
}

static char *dir_of(const char *path) {
    const char *slash = NULL;
    for (const char *p = path; *p; p++)
        if (*p == '/') slash = p;
    if (!slash) return xstrdup(".");
    if (slash == path) return xstrdup("/");
    size_t n = (size_t)(slash - path);
    char *d = malloc(n + 1);
    if (!d) return NULL;
    memcpy(d, path, n);
    d[n] = '\0';
    return d;
}

static char *read_file_text(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc((size_t)size + 1);
    if (!buf) { fclose(f); return NULL; }
    size_t nread = fread(buf, 1, (size_t)size, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

static char *find_runtime_dir(const char *hint) {
    if (hint && hint[0]) {
        char *probe = path_join(hint, "string.c");
        int ok = file_readable(probe);
        free(probe);
        if (ok) return xstrdup(hint);
    }
    const char *env = getenv("FAKECC_RT");
    if (env && env[0]) {
        char *probe = path_join(env, "string.c");
        int ok = file_readable(probe);
        free(probe);
        if (ok) return xstrdup(env);
    }
    if (file_readable("runtime/string.c")) return xstrdup("runtime");

    const char *sys_paths[] = {
        "/usr/local/share/fakecc/runtime",
        "/usr/share/fakecc/runtime",
        NULL
    };
    for (int i = 0; sys_paths[i]; i++) {
        char *probe = path_join(sys_paths[i], "string.c");
        int ok = file_readable(probe);
        free(probe);
        if (ok) return xstrdup(sys_paths[i]);
    }
    return NULL;
}

static int ends_with(const char *s, const char *suffix) {
    size_t slen = strlen(s), suflen = strlen(suffix);
    if (slen < suflen) return 0;
    return strcmp(s + slen - suflen, suffix) == 0;
}

static int lower_one_tu(TranslationUnit *tu, const char *filename,
                        EmitModule *out, int opt_level, int want_debug,
                        PkgContext *pkg) {
    int rc;
    if (pkg)
        rc = sema_check_in_pkg(tu, 0, pkg);
    else
        rc = sema_check(tu, 0);
    if (rc != FAKECC_OK || sema_has_errors())
        return FAKECC_ERR;

    IRModule ir;
    ir_module_init(&ir);
    if (ir_generate(tu, &ir, opt_level == 0) != FAKECC_OK) {
        ir_module_free(&ir);
        return FAKECC_ERR;
    }

    opt(&ir, opt_level, want_debug);

    emit_module_init(out);
    if (want_debug) out->dbg_tu_name = xstrdup(filename);
    codegen(&ir, out, want_debug);

    ir_module_free(&ir);
    return FAKECC_OK;
}

/* Lex + parse a source string. Returns FAKECC_OK / FAKECC_ERR. */
static int compile_parse_string(const char *source, const char *filename,
                                TranslationUnit *tu, PkgContext *pkg) {
    TokenArray tokens;
    token_array_init(&tokens);
    if (lex(source, filename, &tokens) != FAKECC_OK) {
        token_array_free(&tokens);
        return FAKECC_ERR;
    }
    tu_init(tu);
    int rc = pkg ? parse_in_pkg(&tokens, tu, pkg) : parse(&tokens, tu);
    token_array_free(&tokens);
    return rc;
}

void fakecc_options_init(FakeccOptions *opts) {
    if (!opts) return;
    memset(opts, 0, sizeof(*opts));
    opts->opt_level = 1;
}

int fakecc_compile_string_to_obj(const char *source,
                                 const char *output_obj_path,
                                 const FakeccOptions *opts) {
    if (!source || !output_obj_path) return -1;
    int opt_level = opts ? opts->opt_level : 1;
    int want_debug = opts ? opts->want_debug : 0;

    TranslationUnit tu;
    if (compile_parse_string(source, "input.c", &tu, NULL) != FAKECC_OK)
        return FAKECC_ERR;

    EmitModule em;
    if (lower_one_tu(&tu, "input.c", &em, opt_level, want_debug, NULL) != FAKECC_OK) {
        tu_free(&tu);
        return FAKECC_ERR;
    }
    tu_free(&tu);

    emit_obj(&em, output_obj_path);
    emit_module_free(&em);
    return FAKECC_OK;
}

int fakecc_compile_string_to_so(const char *source,
                                const char *output_so_path,
                                const FakeccOptions *opts) {
    if (!source || !output_so_path) return -1;
    int opt_level = opts ? opts->opt_level : 1;
    int want_debug = opts ? opts->want_debug : 0;

    TranslationUnit tu;
    if (compile_parse_string(source, "input.c", &tu, NULL) != FAKECC_OK)
        return FAKECC_ERR;

    EmitModule em;
    if (lower_one_tu(&tu, "input.c", &em, opt_level, want_debug, NULL) != FAKECC_OK) {
        tu_free(&tu);
        return FAKECC_ERR;
    }
    tu_free(&tu);

    EmitModule *mods[1];
    mods[0] = &em;

    const char **needed = opts ? opts->needed : NULL;
    size_t num_needed = opts ? opts->num_needed : 0;
    const char **lib_paths = opts ? opts->lib_paths : NULL;
    size_t num_lib_paths = opts ? opts->num_lib_paths : 0;

    emit_link(mods, 1, output_so_path,
              needed, num_needed, 0,
              lib_paths, num_lib_paths,
              want_debug, 1);

    emit_module_free(&em);
    return FAKECC_OK;
}

int fakecc_compile_string_to_executable(const char *source,
                                        const char *output_path,
                                        const FakeccOptions *opts) {
    if (!source || !output_path) return -1;
    int opt_level = opts ? opts->opt_level : 1;
    int want_debug = opts ? opts->want_debug : 0;
    int nostdlib = opts ? opts->nostdlib : 0;

    const char **needed = opts ? opts->needed : NULL;
    size_t num_needed = opts ? opts->num_needed : 0;
    const char **lib_paths = opts ? opts->lib_paths : NULL;
    size_t num_lib_paths = opts ? opts->num_lib_paths : 0;

    if (nostdlib) {
        TranslationUnit tu;
        if (compile_parse_string(source, "input.c", &tu, NULL) != FAKECC_OK)
            return FAKECC_ERR;

        EmitModule em;
        if (lower_one_tu(&tu, "input.c", &em, opt_level, want_debug, NULL) != FAKECC_OK) {
            tu_free(&tu);
            return FAKECC_ERR;
        }
        tu_free(&tu);

        EmitModule *mods[1];
        mods[0] = &em;
        emit_link(mods, 1, output_path,
                  needed, num_needed, 0,
                  lib_paths, num_lib_paths,
                  want_debug, 0);

        emit_module_free(&em);
        return FAKECC_OK;
    }

    char *rt_dir = find_runtime_dir(opts ? opts->rt_dir : NULL);
    if (!rt_dir) return -1;

    PkgContext pkg;
    pkg_ctx_init(&pkg);

    char *rt_parent = dir_of(rt_dir);
    if (rt_parent) {
        pkg_ctx_add_path(&pkg, rt_parent);
        free(rt_parent);
    }
    free(rt_dir);

    SourceLoc zloc = {0};
    pkg_load(&pkg, "runtime", zloc);

    TranslationUnit tu;
    if (compile_parse_string(source, "input.c", &tu, &pkg) != FAKECC_OK) {
        pkg_ctx_free(&pkg);
        return FAKECC_ERR;
    }

    int nlinked = 0;
    for (size_t p = 0; p < pkg.npkgs; p++) {
        if (!pkg.pkgs[p]->owns_files) continue;
        nlinked += (int)pkg.pkgs[p]->nfiles;
    }

    int total_mods = 1 + nlinked;
    EmitModule *all_mods = malloc((size_t)total_mods * sizeof(EmitModule));
    EmitModule **mod_ptrs = malloc((size_t)total_mods * sizeof(EmitModule *));
    if (!all_mods || !mod_ptrs) {
        tu_free(&tu);
        pkg_ctx_free(&pkg);
        free(all_mods);
        free(mod_ptrs);
        return -1;
    }

    if (lower_one_tu(&tu, "input.c", &all_mods[0], opt_level, want_debug, &pkg) != FAKECC_OK) return FAKECC_ERR;
    tu_free(&tu);
    mod_ptrs[0] = &all_mods[0];

    int mi = 1;
    for (size_t p = 0; p < pkg.npkgs; p++) {
        Package *pp = pkg.pkgs[p];
        if (!pp->owns_files) continue;
        for (size_t f = 0; f < pp->nfiles; f++) {
            char *fake = path_join(pp->dir, "_.c");
            if (lower_one_tu(&pp->files[f], fake, &all_mods[mi], opt_level, want_debug, &pkg) != FAKECC_OK) return FAKECC_ERR;
            free(fake);
            mod_ptrs[mi] = &all_mods[mi];
            mi++;
        }
    }

    emit_link(mod_ptrs, (size_t)total_mods, output_path,
              needed, num_needed, 0,
              lib_paths, num_lib_paths,
              want_debug, 0);

    for (int i = 0; i < total_mods; i++) emit_module_free(&all_mods[i]);
    free(all_mods);
    free(mod_ptrs);
    pkg_ctx_free(&pkg);
    return 0;
}

int fakecc_compile_file_to_obj(const char *source_path,
                               const char *output_obj_path,
                               const FakeccOptions *opts) {
    char *text = read_file_text(source_path);
    if (!text) return -1;
    int rc = fakecc_compile_string_to_obj(text, output_obj_path, opts);
    free(text);
    return rc;
}

int fakecc_compile_file_to_so(const char *source_path,
                              const char *output_so_path,
                              const FakeccOptions *opts) {
    return fakecc_compile_files(&source_path, 1, output_so_path, 1, opts);
}

int fakecc_compile_file_to_executable(const char *source_path,
                                      const char *output_path,
                                      const FakeccOptions *opts) {
    return fakecc_compile_files(&source_path, 1, output_path, 0, opts);
}

int fakecc_compile_files(const char **input_paths,
                         size_t num_inputs,
                         const char *output_path,
                         int is_shared,
                         const FakeccOptions *opts) {
    if (!input_paths || num_inputs == 0 || !output_path) return -1;

    int opt_level = opts ? opts->opt_level : 1;
    int want_debug = opts ? opts->want_debug : 0;
    int nostdlib = opts ? opts->nostdlib : (is_shared ? 1 : 0);

    const char **needed = opts ? opts->needed : NULL;
    size_t num_needed = opts ? opts->num_needed : 0;
    const char **lib_paths = opts ? opts->lib_paths : NULL;
    size_t num_lib_paths = opts ? opts->num_lib_paths : 0;

    PkgContext pkg;
    pkg_ctx_init(&pkg);

    for (size_t i = 0; i < num_inputs; i++) {
        if (!ends_with(input_paths[i], ".o")) {
            char *d = dir_of(input_paths[i]);
            if (d) {
                pkg_ctx_add_path(&pkg, d);
                free(d);
            }
        }
    }

    if (!nostdlib) {
        char *rt_dir = find_runtime_dir(opts ? opts->rt_dir : NULL);
        if (!rt_dir) {
            pkg_ctx_free(&pkg);
            return -1;
        }
        char *rt_parent = dir_of(rt_dir);
        if (rt_parent) {
            pkg_ctx_add_path(&pkg, rt_parent);
            free(rt_parent);
        }
        free(rt_dir);
        SourceLoc zloc = {0};
        pkg_load(&pkg, "runtime", zloc);
    }

    TranslationUnit *user_tus = malloc(num_inputs * sizeof(TranslationUnit));
    if (!user_tus) { pkg_ctx_free(&pkg); return -1; }

    for (size_t i = 0; i < num_inputs; i++) {
        tu_init(&user_tus[i]);
        if (!ends_with(input_paths[i], ".o")) {
            char *src = read_file_text(input_paths[i]);
            if (!src) {
                for (size_t j = 0; j <= i; j++) tu_free(&user_tus[j]);
                free(user_tus);
                pkg_ctx_free(&pkg);
                return -1;
            }
            TokenArray arr;
            token_array_init(&arr);
            if (lex(src, input_paths[i], &arr) != FAKECC_OK) {
                free(src);
                token_array_free(&arr);
                for (size_t j = 0; j <= i; j++) tu_free(&user_tus[j]);
                free(user_tus);
                pkg_ctx_free(&pkg);
                return FAKECC_ERR;
            }
            free(src);

            if (parse_in_pkg(&arr, &user_tus[i], &pkg) != FAKECC_OK) {
                token_array_free(&arr);
                for (size_t j = 0; j <= i; j++) tu_free(&user_tus[j]);
                free(user_tus);
                pkg_ctx_free(&pkg);
                return FAKECC_ERR;
            }
            token_array_free(&arr);

            if (user_tus[i].package.name) {
                TranslationUnit *one = &user_tus[i];
                pkg_register_tus(&pkg, user_tus[i].package.name, &one, 1);
            }
        }
    }

    int nlinked = 0;
    for (size_t p = 0; p < pkg.npkgs; p++) {
        if (!pkg.pkgs[p]->owns_files) continue;
        nlinked += (int)pkg.pkgs[p]->nfiles;
    }

    size_t total_mods = num_inputs + (size_t)nlinked;
    EmitModule *mods = malloc(total_mods * sizeof(EmitModule));
    EmitModule **mod_ptrs = malloc(total_mods * sizeof(EmitModule *));
    if (!mods || !mod_ptrs) {
        for (size_t i = 0; i < num_inputs; i++) tu_free(&user_tus[i]);
        free(user_tus);
        free(mods);
        free(mod_ptrs);
        pkg_ctx_free(&pkg);
        return -1;
    }

    for (size_t i = 0; i < num_inputs; i++) {
        if (ends_with(input_paths[i], ".o")) {
            if (emit_obj_read(input_paths[i], &mods[i]) != 0) {
                for (size_t j = 0; j < i; j++) emit_module_free(&mods[j]);
                for (size_t j = 0; j < num_inputs; j++) tu_free(&user_tus[j]);
                free(user_tus);
                free(mods);
                free(mod_ptrs);
                pkg_ctx_free(&pkg);
                return -1;
            }
        } else {
            if (lower_one_tu(&user_tus[i], input_paths[i], &mods[i], opt_level, want_debug, &pkg) != FAKECC_OK) return FAKECC_ERR;
            tu_free(&user_tus[i]);
        }
        mod_ptrs[i] = &mods[i];
    }
    free(user_tus);

    size_t mi = num_inputs;
    for (size_t p = 0; p < pkg.npkgs; p++) {
        Package *pp = pkg.pkgs[p];
        if (!pp->owns_files) continue;
        for (size_t f = 0; f < pp->nfiles; f++) {
            char *fake = path_join(pp->dir, "_.c");
            if (lower_one_tu(&pp->files[f], fake, &mods[mi], opt_level, want_debug, &pkg) != FAKECC_OK) return FAKECC_ERR;
            free(fake);
            mod_ptrs[mi] = &mods[mi];
            mi++;
        }
    }

    emit_link(mod_ptrs, total_mods, output_path,
              needed, num_needed, 0,
              lib_paths, num_lib_paths,
              want_debug, is_shared);

    for (size_t i = 0; i < total_mods; i++) emit_module_free(&mods[i]);
    free(mods);
    free(mod_ptrs);
    pkg_ctx_free(&pkg);
    return 0;
}
