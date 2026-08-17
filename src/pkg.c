#define _GNU_SOURCE
#include "fakecc/pkg.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Directory listing: Stage0 (gcc) uses libc open/getdents; the self-hosted
 * dialect has no libc, so translate.py defines FAKECC_SELFHOST and we go
 * through __syscall. */
#ifdef FAKECC_SELFHOST
#define PKG_O_RDONLY    0
#define PKG_O_DIRECTORY 65536
static long pkg_open(const char *path, long flags) {
    return __syscall(2, (long)path, flags, 0);
}
static long pkg_close(long fd) { return __syscall(3, fd); }
static long pkg_getdents(long fd, void *buf, long n) {
    return __syscall(217, fd, (long)buf, n);
}
#else
#include <fcntl.h>
#include <unistd.h>
#include <sys/syscall.h>
#ifndef O_DIRECTORY
#define O_DIRECTORY 65536
#endif
#define PKG_O_RDONLY    O_RDONLY
#define PKG_O_DIRECTORY O_DIRECTORY
static long pkg_open(const char *path, long flags) {
    return (long)open(path, (int)flags);
}
static long pkg_close(long fd) { return (long)close((int)fd); }
static long pkg_getdents(long fd, void *buf, long n) {
    return syscall(SYS_getdents64, (int)fd, buf, (unsigned long)n);
}
#endif

/* Linux dirent64 layout (getdents64). d_name is a flexible trailing field;
 * we declare [1] so both gcc and the FakeCC dialect accept the type. */
struct pkg_dirent64 {
    unsigned long long d_ino;
    long long d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[1];
};
/* ------------------------------------------------------------------ */
/* Context lifetime                                                    */
/* ------------------------------------------------------------------ */

void pkg_ctx_init(PkgContext *ctx) {
    memset(ctx, 0, sizeof(*ctx));
}

void pkg_ctx_free(PkgContext *ctx) {
    for (int i = 0; i < ctx->npaths; i++) free(ctx->search_paths[i]);
    free(ctx->search_paths);
    for (size_t i = 0; i < ctx->npkgs; i++) {
        Package *p = ctx->pkgs[i];
        free(p->name);
        free(p->dir);
        if (p->owns_files) {
            for (size_t f = 0; f < p->nfiles; f++)
                tu_free(&p->files[f]);
        }
        free(p->files);
        for (size_t j = 0; j < p->nfuncs; j++) {
            free(p->funcs[j].name);
            type_free(&p->funcs[j].ret_type);
            for (int k = 0; k < p->funcs[j].arity && k < 16; k++)
                type_free(&p->funcs[j].param_types[k]);
        }
        free(p->funcs);
        for (size_t j = 0; j < p->nglobals; j++) {
            free(p->globals[j].name);
            type_free(&p->globals[j].type);
        }
        free(p->globals);
        typedef_registry_free(&p->typedefs);
        struct_registry_free(&p->structs);
        enum_registry_free(&p->enums);
        free(p);
    }
    free(ctx->pkgs);
    for (size_t i = 0; i < ctx->nloading; i++) free(ctx->loading[i]);
    free(ctx->loading);
    memset(ctx, 0, sizeof(*ctx));
}

void pkg_ctx_add_path(PkgContext *ctx, const char *dir) {
    if (!dir || !dir[0]) return;
    for (int i = 0; i < ctx->npaths; i++)
        if (strcmp(ctx->search_paths[i], dir) == 0) return;
    ctx->search_paths = xrealloc(ctx->search_paths,
                                 (size_t)(ctx->npaths + 1) * sizeof(char *));
    ctx->search_paths[ctx->npaths++] = xstrdup(dir);
}

Package *pkg_find(const PkgContext *ctx, const char *name) {
    for (size_t i = 0; i < ctx->npkgs; i++)
        if (strcmp(ctx->pkgs[i]->name, name) == 0) return ctx->pkgs[i];
    return NULL;
}

const PkgFuncExport *pkg_find_func(const Package *pkg, const char *name) {
    for (size_t i = 0; i < pkg->nfuncs; i++)
        if (strcmp(pkg->funcs[i].name, name) == 0) return &pkg->funcs[i];
    return NULL;
}

const PkgGlobalExport *pkg_find_global(const Package *pkg, const char *name) {
    for (size_t i = 0; i < pkg->nglobals; i++)
        if (strcmp(pkg->globals[i].name, name) == 0) return &pkg->globals[i];
    return NULL;
}

const Type *pkg_find_typedef(const Package *pkg, const char *name) {
    return typedef_registry_find(&pkg->typedefs, name);
}

const StructDef *pkg_find_struct(const Package *pkg, const char *name) {
    return struct_registry_find_c(&pkg->structs, name);
}

const EnumDef *pkg_find_enum(const Package *pkg, const char *name) {
    return enum_registry_find((EnumRegistry *)&pkg->enums, name);
}

const EnumConstant *pkg_find_enum_const(const Package *pkg, const char *name) {
    return enum_registry_find_constant(&pkg->enums, name);
}

const char *pkg_suggest_export(const PkgContext *ctx, const char *name) {
    for (size_t i = 0; i < ctx->npkgs; i++) {
        Package *p = ctx->pkgs[i];
        if (pkg_find_func(p, name) || pkg_find_global(p, name)
            || pkg_find_typedef(p, name))
            return p->name;
    }
    return NULL;
}

/* ------------------------------------------------------------------ */
/* Struct / typedef cloning                                            */
/* ------------------------------------------------------------------ */

void pkg_clone_struct_into(StructRegistry *dst, const StructDef *src) {
    StructDef *exist = struct_registry_find(dst, src->tag);
    if (exist) {
        if (exist->num_members == src->num_members
            && exist->size == src->size
            && exist->is_union == src->is_union)
            return; /* already present with matching shape */
        die_at(src->loc.file, src->loc.line, src->loc.col,
               "conflicting definitions of '%s%s'",
               src->is_union ? "union " : "struct ", src->tag);
    }
    StructDef *sd = struct_registry_add(dst, src->tag, src->loc);
    sd->is_union = src->is_union;
    for (int i = 0; i < src->num_members; i++) {
        struct_def_push_member(sd, src->members[i].name,
                               type_clone(src->members[i].type),
                               src->members[i].bit_width);
        /* Preserve packed bitfield offsets from the source layout. */
        if (src->members[i].bit_width > 0) {
            sd->members[sd->num_members - 1].offset = src->members[i].offset;
            sd->members[sd->num_members - 1].bit_offset = src->members[i].bit_offset;
        }
    }
    /* Prefer the source's finished size/align over re-layout, so incomplete
     * clones and bitfield packs stay ABI-identical. */
    sd->size = src->size;
    sd->align = src->align;
    if (sd->canonical_type) {
        sd->canonical_type->width = sd->size;
    }
}

void pkg_import_typedef(TranslationUnit *tu, const char *name, const Type *src,
                        const Package *pkg) {
    const Type *exist = typedef_registry_find(&tu->typedefs, name);
    if (exist) return; /* local wins; plan: never inject over a local decl */
    Type t = type_clone(*src);
    if (t.kind == TY_STRUCT && t.tag) {
        const StructDef *sd = pkg_find_struct(pkg, t.tag);
        if (!sd)
            sd = struct_registry_find_c(&tu->structs, t.tag);
        if (sd)
            pkg_clone_struct_into(&tu->structs, sd);
        /* Refresh width from the (possibly just-cloned) local registry. */
        StructDef *local = struct_registry_find(&tu->structs, t.tag);
        if (local) t.width = local->size;
    } else if (t.kind == TY_PTR && t.pointee && t.pointee->kind == TY_STRUCT
               && t.pointee->tag) {
        const StructDef *sd = pkg_find_struct(pkg, t.pointee->tag);
        if (sd)
            pkg_clone_struct_into(&tu->structs, sd);
    }
    typedef_registry_add(&tu->typedefs, name, t);
}

/* ------------------------------------------------------------------ */
/* Helpers                                                             */
/* ------------------------------------------------------------------ */

static char *path_join(const char *a, const char *b) {
    size_t na = strlen(a), nb = strlen(b);
    int slash = (na > 0 && a[na - 1] != '/');
    char *p = xmalloc(na + (size_t)slash + nb + 1);
    memcpy(p, a, na);
    if (slash) p[na++] = '/';
    memcpy(p + na, b, nb + 1);
    return p;
}

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = xmalloc((size_t)size + 1);
    size_t nread = fread(buf, 1, (size_t)size, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

static int ends_with_c(const char *name) {
    size_t n = strlen(name);
    return n >= 2 && name[n - 2] == '.' && name[n - 1] == 'c';
}

static int is_dir(const char *path) {
    long fd = pkg_open(path, PKG_O_RDONLY | PKG_O_DIRECTORY);
    if (fd < 0) return 0;
    pkg_close(fd);
    return 1;
}

/* True if dir contains at least one *.c file. */
static int dir_has_c(const char *dir) {
    long fd = pkg_open(dir, PKG_O_RDONLY | PKG_O_DIRECTORY);
    if (fd < 0) return 0;
    char buf[2048];
    int found = 0;
    for (;;) {
        long nread = pkg_getdents(fd, buf, (long)sizeof buf);
        if (nread <= 0) break;
        long off = 0;
        while (off < nread) {
            struct pkg_dirent64 *e = (struct pkg_dirent64 *)(buf + off);
            if (ends_with_c(e->d_name)) { found = 1; break; }
            off = off + e->d_reclen;
        }
        if (found) break;
    }
    pkg_close(fd);
    return found;
}

/* Find <search>/name/ that contains at least one .c file. Caller frees. */
static char *find_pkg_dir(PkgContext *ctx, const char *name) {
    for (int i = 0; i < ctx->npaths; i++) {
        char *cand = path_join(ctx->search_paths[i], name);
        if (is_dir(cand) && dir_has_c(cand))
            return cand;
        free(cand);
    }
    return NULL;
}

static void loading_push(PkgContext *ctx, const char *name) {
    if (ctx->nloading >= ctx->cap_loading) {
        ctx->cap_loading = ctx->cap_loading ? ctx->cap_loading * 2 : 4;
        ctx->loading = xrealloc(ctx->loading, ctx->cap_loading * sizeof(char *));
    }
    ctx->loading[ctx->nloading++] = xstrdup(name);
}

static void loading_pop(PkgContext *ctx) {
    if (ctx->nloading == 0) return;
    free(ctx->loading[--ctx->nloading]);
}

static int loading_contains(const PkgContext *ctx, const char *name) {
    for (size_t i = 0; i < ctx->nloading; i++)
        if (strcmp(ctx->loading[i], name) == 0) return 1;
    return 0;
}

static void pkgs_push(PkgContext *ctx, Package *p) {
    if (ctx->npkgs >= ctx->cap_pkgs) {
        ctx->cap_pkgs = ctx->cap_pkgs ? ctx->cap_pkgs * 2 : 4;
        ctx->pkgs = xrealloc(ctx->pkgs, ctx->cap_pkgs * sizeof(Package *));
    }
    ctx->pkgs[ctx->npkgs++] = p;
}

/* Collect *.c basenames in dir, sorted. Caller frees names and the array. */
static char **list_c_files(const char *dir, size_t *nout) {
    long fd = pkg_open(dir, PKG_O_RDONLY | PKG_O_DIRECTORY);
    if (fd < 0) {
        fprintf(stderr, "fakecc: cannot open package directory '%s'\n", dir);
        exit(1);
    }
    char **names = NULL;
    size_t n = 0, cap = 0;
    char buf[2048];
    for (;;) {
        long nread = pkg_getdents(fd, buf, (long)sizeof buf);
        if (nread <= 0) break;
        long off = 0;
        while (off < nread) {
            struct pkg_dirent64 *e = (struct pkg_dirent64 *)(buf + off);
            if (ends_with_c(e->d_name)) {
                if (n >= cap) {
                    cap = cap ? cap * 2 : 4;
                    names = xrealloc(names, cap * sizeof(char *));
                }
                names[n++] = xstrdup(e->d_name);
            }
            off = off + e->d_reclen;
        }
    }
    pkg_close(fd);
    /* Insertion sort — packages are tiny. */
    for (size_t i = 1; i < n; i++) {
        char *key = names[i];
        size_t j = i;
        while (j > 0 && strcmp(names[j - 1], key) > 0) {
            names[j] = names[j - 1];
            j--;
        }
        names[j] = key;
    }
    *nout = n;
    return names;
}

/* Fold one TU's non-static decls into the package export tables (first wins). */
static void add_tu_exports(Package *pkg, TranslationUnit *tu) {
    for (size_t i = 0; i < tu->functions.len; i++) {
        FunctionDecl *fn = &tu->functions.data[i];
        if (fn->is_static) continue;
        if (pkg_find_func(pkg, fn->name)) continue;
        pkg->funcs = xrealloc(pkg->funcs,
                              (pkg->nfuncs + 1) * sizeof(PkgFuncExport));
        PkgFuncExport *e = &pkg->funcs[pkg->nfuncs++];
        memset(e, 0, sizeof(*e));
        e->name = xstrdup(fn->name);
        e->ret_type = type_clone(fn->ret_type);
        e->arity = (int)fn->params.len;
        e->is_variadic = fn->is_variadic;
        e->is_extern = fn->is_extern;
        e->loc = fn->loc;
        e->tu = tu;
        for (int k = 0; k < e->arity && k < 16; k++)
            e->param_types[k] = type_clone(fn->params.data[k].type);
    }

    for (size_t i = 0; i < tu->globals.len; i++) {
        Stmt *s = &tu->globals.data[i];
        if (s->kind != ST_DECL) continue;
        if (s->u.decl.storage_class == 1) continue; /* static */
        if (pkg_find_global(pkg, s->u.decl.name)) continue;
        pkg->globals = xrealloc(pkg->globals,
                                (pkg->nglobals + 1) * sizeof(PkgGlobalExport));
        PkgGlobalExport *e = &pkg->globals[pkg->nglobals++];
        memset(e, 0, sizeof(*e));
        e->name = xstrdup(s->u.decl.name);
        e->type = type_clone(s->u.decl.type);
        e->is_extern = (s->u.decl.storage_class == 2);
        e->loc = s->loc;
        e->tu = tu;
    }

    /* Skip compiler-injected va_list / __va_list_tag — every TU has its own
     * copy from tu_init. */
    for (size_t i = 0; i < tu->typedefs.len; i++) {
        TypedefEntry *te = &tu->typedefs.data[i];
        if (strcmp(te->name, "va_list") == 0) continue;
        if (!typedef_registry_find(&pkg->typedefs, te->name))
            typedef_registry_add(&pkg->typedefs, te->name, type_clone(te->type));
    }
    for (size_t i = 0; i < tu->structs.len; i++) {
        StructDef *sd = &tu->structs.data[i];
        if (strcmp(sd->tag, "__va_list_tag") == 0) continue;
        if (sd->tag && strncmp(sd->tag, "__anon_", 7) == 0) continue;
        if (!struct_registry_find(&pkg->structs, sd->tag))
            pkg_clone_struct_into(&pkg->structs, sd);
    }
    for (size_t i = 0; i < tu->enums.len; i++) {
        EnumDef *ed = &tu->enums.data[i];
        if (!ed->tag) continue;
        if (ed->tag && strncmp(ed->tag, "__anon_", 7) == 0) continue;
        if (!enum_registry_find(&pkg->enums, ed->tag)) {
            EnumDef *ne = enum_registry_add(&pkg->enums, ed->tag, ed->loc);
            for (int c = 0; c < ed->num_constants; c++)
                enum_def_push_constant(ne, ed->constants[c].name, 1,
                                       ed->constants[c].value, ed->loc);
        }
    }
}

static void build_exports(Package *pkg) {
    typedef_registry_init(&pkg->typedefs);
    struct_registry_init(&pkg->structs);
    enum_registry_init(&pkg->enums);
    pkg->funcs = NULL;
    pkg->nfuncs = 0;
    pkg->globals = NULL;
    pkg->nglobals = 0;

    for (size_t f = 0; f < pkg->nfiles; f++)
        add_tu_exports(pkg, &pkg->files[f]);
}

/* ------------------------------------------------------------------ */
/* Load                                                                */
/* ------------------------------------------------------------------ */

Package *pkg_load(PkgContext *ctx, const char *name, SourceLoc loc) {
    /* Cycle check before cache: an in-progress package is already in the
     * cache shell (so siblings can see it), so a reverse import must die
     * here rather than returning the incomplete Package. */
    if (loading_contains(ctx, name)) {
        die_at(loc.file, loc.line, loc.col,
               "import cycle involving package '%s'", name);
    }

    Package *cached = pkg_find(ctx, name);
    if (cached) return cached;

    char *dir = find_pkg_dir(ctx, name);
    if (!dir) {
        die_at(loc.file, loc.line, loc.col,
               "package '%s' not found (search path has %d entries)",
               name, ctx->npaths);
    }

    loading_push(ctx, name);

    size_t nnames = 0;
    char **names = list_c_files(dir, &nnames);
    if (nnames == 0) {
        die_at(loc.file, loc.line, loc.col,
               "package '%s' directory '%s' has no .c files", name, dir);
    }

    Package *pkg = xmalloc(sizeof(Package));
    memset(pkg, 0, sizeof(*pkg));
    pkg->name = xstrdup(name);
    pkg->dir = dir;
    pkg->files = xmalloc(nnames * sizeof(TranslationUnit));
    memset(pkg->files, 0, nnames * sizeof(TranslationUnit));
    pkg->nfiles = nnames;
    pkg->owns_files = 1;

    /* Register early so same-package sibling lookups during parse can see
     * the Package shell (files filled in as we go). Exports come later. */
    pkgs_push(ctx, pkg);

    TokenArray *all_tokens = xmalloc(nnames * sizeof(TokenArray));
    for (size_t i = 0; i < nnames; i++) {
        char *path = path_join(dir, names[i]);
        char *src = read_file(path);
        token_array_init(&all_tokens[i]);
        lex(src, path, &all_tokens[i]);
        tu_init(&pkg->files[i]);
        free(src);
        (void)path;
    }

    for (size_t i = 0; i < nnames; i++) {
        /* parse_in_pkg resolves imports recursively via this same ctx. */
        parse_in_pkg(&all_tokens[i], &pkg->files[i], ctx);

        if (!pkg->files[i].package.name
            || strcmp(pkg->files[i].package.name, name) != 0) {
            die_at(pkg->files[i].package.loc.file,
                   pkg->files[i].package.loc.line,
                   pkg->files[i].package.loc.col,
                   "package name '%s' does not match directory '%s'",
                   pkg->files[i].package.name
                       ? pkg->files[i].package.name : "(none)",
                   name);
        }

        token_array_free(&all_tokens[i]);
        free(names[i]);
    }
    free(all_tokens);
    free(names);

    build_exports(pkg);
    loading_pop(ctx);
    return pkg;
}

Package *pkg_register_tus(PkgContext *ctx, const char *name,
                          TranslationUnit **tus, size_t ntus) {
    if (!name || !ntus) return NULL;
    Package *exist = pkg_find(ctx, name);
    if (exist) {
        /* Merge exports (first name wins).  Do not append to exist->files:
         * directory packages own those TUs; CLI TUs stay with the driver. */
        for (size_t i = 0; i < ntus; i++)
            add_tu_exports(exist, tus[i]);
        return exist;
    }
    Package *pkg = xmalloc(sizeof(Package));
    memset(pkg, 0, sizeof(*pkg));
    pkg->name = xstrdup(name);
    pkg->dir = NULL;
    pkg->owns_files = 0;
    /* Shallow-copy TU structs so build_exports can index them contiguously.
     * Heap ownership of AST nodes stays with the caller's TUs. */
    pkg->files = xmalloc(ntus * sizeof(TranslationUnit));
    pkg->nfiles = ntus;
    for (size_t i = 0; i < ntus; i++)
        pkg->files[i] = *tus[i];
    pkgs_push(ctx, pkg);
    build_exports(pkg);
    /* Exports are deep-cloned into pkg->funcs/globals/typedefs/...; the shallow
     * copies are no longer needed.  Drop them now so they never dangle once the
     * driver frees its TUs in Phase 2 (owns_files is already 0, so pkg_ctx_free
     * will not free them).  After this the package is identified as a user
     * package solely by owns_files == 0. */
    free(pkg->files);
    pkg->files = NULL;
    pkg->nfiles = 0;
    return pkg;
}
