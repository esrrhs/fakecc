#ifndef FAKECC_PKG_H
#define FAKECC_PKG_H

#include "fakecc/ast.h"
#include "fakecc/common.h"

/* Package loader: directory-per-package, Go-style `import name;`.
 * Qualification (fmt.printf) is a front-end concept only — ELF symbols stay
 * unmangled, so an imported name resolves to the same linker symbol as an
 * `extern` declaration of the same identifier. */

typedef struct Package Package;
typedef struct PkgContext PkgContext;

/* One exported top-level function (non-static). */
typedef struct {
    char *name;
    Type ret_type;          /* cloned */
    Type param_types[16];   /* cloned; unused slots zeroed */
    int arity;
    int is_variadic;
    int is_extern;          /* declaration-only */
    SourceLoc loc;
    TranslationUnit *tu;    /* defining file (not owned) */
} PkgFuncExport;

/* One exported top-level global (non-static). */
typedef struct {
    char *name;
    Type type;              /* cloned */
    int is_extern;
    SourceLoc loc;
    TranslationUnit *tu;
} PkgGlobalExport;

struct Package {
    char *name;                 /* package name (= directory basename) */
    char *dir;                  /* absolute-ish path used to load it */
    TranslationUnit *files;     /* array of TUs, one per .c */
    size_t nfiles;
    int owns_files;             /* 1 = pkg_ctx_free calls tu_free on each file */
    PkgFuncExport *funcs;
    size_t nfuncs;
    PkgGlobalExport *globals;
    size_t nglobals;
    /* Aggregated type registries: clones of non-private typedefs/structs/enums
     * from every file. Used for qualified type names (io.FILE) and for
     * same-package unqualified fallback. */
    TypedefRegistry typedefs;
    StructRegistry structs;
    EnumRegistry enums;
};

struct PkgContext {
    char **search_paths;        /* owned; searched in order */
    int npaths;
    Package **pkgs;             /* owned pointers; cache of loaded packages */
    size_t npkgs;
    size_t cap_pkgs;
    /* Stack of package names currently being loaded (cycle detection). */
    char **loading;
    size_t nloading;
    size_t cap_loading;
};

void pkg_ctx_init(PkgContext *ctx);
void pkg_ctx_free(PkgContext *ctx);
/* Append a directory to the search path (deduped). */
void pkg_ctx_add_path(PkgContext *ctx, const char *dir);

/* Load package `name` (or return the cached copy). Dies on missing package,
 * package/directory name mismatch, or import cycle. `loc` is the import site
 * used in diagnostics (may be zeroed for driver-initiated loads). */
Package *pkg_load(PkgContext *ctx, const char *name, SourceLoc loc);

/* Register already-parsed TUs as package `name` for same-package visibility.
 * Does not take ownership of the TUs (caller frees them). Dies if `name` is
 * already loaded (e.g. a directory package). */
Package *pkg_register_tus(PkgContext *ctx, const char *name,
                          TranslationUnit **tus, size_t ntus);

/* Look up a previously loaded package; NULL if never loaded. */
Package *pkg_find(const PkgContext *ctx, const char *name);

/* Export-table lookups (NULL / 0 if absent). */
const PkgFuncExport *pkg_find_func(const Package *pkg, const char *name);
const PkgGlobalExport *pkg_find_global(const Package *pkg, const char *name);
const Type *pkg_find_typedef(const Package *pkg, const char *name);
const StructDef *pkg_find_struct(const Package *pkg, const char *name);
const EnumDef *pkg_find_enum(const Package *pkg, const char *name);
const EnumConstant *pkg_find_enum_const(const Package *pkg, const char *name);

/* Clone a StructDef (and its members) into `dst` under the same tag.
 * Dies if `dst` already has a differently-shaped struct with that tag. */
void pkg_clone_struct_into(StructRegistry *dst, const StructDef *src);

/* Clone a typedef (and any referenced struct) into `tu`. Idempotent if the
 * local typedef already matches; dies on conflicting redefinition. */
void pkg_import_typedef(TranslationUnit *tu, const char *name, const Type *src,
                        const Package *pkg);

/* Suggest which package exports `name` (for "did you mean?" diagnostics).
 * Searches loaded packages only. Returns package name or NULL. */
const char *pkg_suggest_export(const PkgContext *ctx, const char *name);

#endif /* FAKECC_PKG_H */
