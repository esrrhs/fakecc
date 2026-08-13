#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/emit.h"
#include "fakecc/ir.h"
#include "fakecc/lexer.h"
#include "fakecc/opt.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static char *read_file(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "fakecc: cannot open '%s'\n", path);
        exit(1);
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char *buf = malloc((size_t)size + 1);
    if (!buf) {
        fprintf(stderr, "fakecc: out of memory\n");
        exit(1);
    }
    size_t nread = fread(buf, 1, (size_t)size, f);
    buf[nread] = '\0';
    fclose(f);
    return buf;
}

/* Compile one .c source into an EmitModule. */
static void compile_source(const char *source, const char *filename,
                           EmitModule *out) {
    TokenArray tokens;
    token_array_init(&tokens);
    lex(source, filename, &tokens);

    TranslationUnit tu;
    tu_init(&tu);
    parse(&tokens, &tu);

    /* The linker verifies `main` globally; individual TUs need not define it. */
    sema_check(&tu, 0);

    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir);

    opt(&ir);

    emit_module_init(out);
    codegen(&ir, out);

    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&tokens);
}

static void module_free(EmitModule *m) {
    emit_module_free(m);
}

static void usage(void) {
    fprintf(stderr,
            "usage: fakecc [-c] [-nodefaultlibs] [-LDIR]... [-lLIB]...\n"
            "              <input...> -o <output>\n"
            "  -lLIB           link against libLIB.so (DT_NEEDED)\n"
            "  -l:SONAME       link against exact soname SONAME\n"
            "  -LDIR           add DIR to the shared-library search path\n"
            "                  (link-time check for -l; also DT_RUNPATH)\n"
            "  -nodefaultlibs  do not auto-link libc.so.6 on dynamic links\n");
    exit(1);
}

static int ends_with(const char *s, const char *suf) {
    size_t n = strlen(s), m = strlen(suf);
    return n >= m && strcmp(s + n - m, suf) == 0;
}

static int is_system_soname(const char *soname) {
    return strcmp(soname, "libc.so.6") == 0
        || strcmp(soname, "libm.so.6") == 0
        || strcmp(soname, "libdl.so.2") == 0
        || strcmp(soname, "libpthread.so.0") == 0;
}

/* Map a -l argument to a DT_NEEDED soname string (caller frees). */
static char *soname_from_l(const char *arg) {
    if (!arg || !*arg) {
        fprintf(stderr, "fakecc: -l requires a library name\n");
        exit(1);
    }
    if (arg[0] == ':')
        return xstrdup(arg + 1);
    /* Common glibc ABI sonames — `libNAME.so` is often a linker script. */
    if (strcmp(arg, "c") == 0) return xstrdup("libc.so.6");
    if (strcmp(arg, "m") == 0) return xstrdup("libm.so.6");
    if (strcmp(arg, "dl") == 0) return xstrdup("libdl.so.2");
    if (strcmp(arg, "pthread") == 0) return xstrdup("libpthread.so.0");
    {
        size_t n = strlen(arg);
        char *s = malloc(n + 8);
        if (!s) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
        memcpy(s, "lib", 3);
        memcpy(s + 3, arg, n);
        memcpy(s + 3 + n, ".so", 4);
        return s;
    }
}

static char *soname_from_so_path(const char *path) {
    const char *base = path;
    for (const char *p = path; *p; p++)
        if (*p == '/') base = p + 1;
    return xstrdup(base);
}

/* Directory containing path ("" for basename-only). Caller frees. */
static char *dir_of(const char *path) {
    const char *slash = NULL;
    for (const char *p = path; *p; p++)
        if (*p == '/') slash = p;
    if (!slash) return xstrdup(".");
    if (slash == path) return xstrdup("/");
    {
        size_t n = (size_t)(slash - path);
        char *d = malloc(n + 1);
        if (!d) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
        memcpy(d, path, n);
        d[n] = '\0';
        return d;
    }
}

static void paths_add(char ***paths, int *n, const char *dir) {
    for (int i = 0; i < *n; i++)
        if (strcmp((*paths)[i], dir) == 0) return;
    *paths = realloc(*paths, ((size_t)*n + 1) * sizeof(char *));
    if (!*paths) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
    (*paths)[(*n)++] = xstrdup(dir);
}

/* True if DIR/SONAME exists as a regular readable file. */
static int lib_in_dir(const char *dir, const char *soname) {
    size_t nd = strlen(dir), ns = strlen(soname);
    char *path = malloc(nd + 1 + ns + 1);
    if (!path) { fprintf(stderr, "fakecc: out of memory\n"); exit(1); }
    memcpy(path, dir, nd);
    path[nd] = '/';
    memcpy(path + nd + 1, soname, ns + 1);
    FILE *f = fopen(path, "rb");
    int ok = f != NULL;
    if (f) fclose(f);
    free(path);
    return ok;
}

/* For non-system -l libs, require a hit in one of the -L directories when any
 * -L was given (gcc-like "cannot find -lfoo"). */
static void require_libs_found(char **needed, int num_needed,
                               char **lib_paths, int num_lib_paths) {
    if (num_lib_paths == 0) return;
    for (int i = 0; i < num_needed; i++) {
        if (is_system_soname(needed[i])) continue;
        int found = 0;
        for (int d = 0; d < num_lib_paths; d++) {
            if (lib_in_dir(lib_paths[d], needed[i])) { found = 1; break; }
        }
        if (!found) {
            fprintf(stderr, "fakecc: cannot find -l library '%s'\n", needed[i]);
            exit(1);
        }
    }
}

int main(int argc, char **argv) {
    int compile_only = 0;
    int nodefaultlibs = 0;
    const char *output_path = NULL;
    const char **inputs = NULL;
    int ninputs = 0;
    char **needed = NULL;
    int num_needed = 0;
    char **lib_paths = NULL;
    int num_lib_paths = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            compile_only = 1;
        } else if (strcmp(argv[i], "-nodefaultlibs") == 0) {
            nodefaultlibs = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) usage();
            output_path = argv[++i];
        } else if (strcmp(argv[i], "-L") == 0) {
            if (i + 1 >= argc) usage();
            paths_add(&lib_paths, &num_lib_paths, argv[++i]);
        } else if (argv[i][0] == '-' && argv[i][1] == 'L') {
            paths_add(&lib_paths, &num_lib_paths, argv[i] + 2);
        } else if (strcmp(argv[i], "-l") == 0) {
            if (i + 1 >= argc) usage();
            needed = realloc(needed, ((size_t)num_needed + 1) * sizeof(char *));
            needed[num_needed++] = soname_from_l(argv[++i]);
        } else if (argv[i][0] == '-' && argv[i][1] == 'l') {
            needed = realloc(needed, ((size_t)num_needed + 1) * sizeof(char *));
            needed[num_needed++] = soname_from_l(argv[i] + 2);
        } else if (ends_with(argv[i], ".so")) {
            /* Pass a .so path: DT_NEEDED = basename; its dir joins -L/RUNPATH. */
            needed = realloc(needed, ((size_t)num_needed + 1) * sizeof(char *));
            needed[num_needed++] = soname_from_so_path(argv[i]);
            {
                char *d = dir_of(argv[i]);
                paths_add(&lib_paths, &num_lib_paths, d);
                free(d);
            }
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "fakecc: unknown option '%s'\n", argv[i]);
            usage();
        } else {
            inputs = realloc(inputs, (ninputs + 1) * sizeof(char *));
            inputs[ninputs++] = argv[i];
        }
    }

    if (ninputs == 0 || output_path == NULL) usage();

    if (compile_only) {
        if (ninputs != 1) {
            fprintf(stderr, "fakecc: -c requires exactly one input\n");
            exit(1);
        }
        if (num_needed > 0 || num_lib_paths > 0 || nodefaultlibs) {
            fprintf(stderr, "fakecc: -l / -L / -nodefaultlibs are link-time options\n");
            exit(1);
        }
        EmitModule em;
        char *src = read_file(inputs[0]);
        compile_source(src, inputs[0], &em);
        free(src);
        emit_obj(&em, output_path);
        module_free(&em);
        free(inputs);
        return 0;
    }

    require_libs_found(needed, num_needed, lib_paths, num_lib_paths);

    EmitModule *mods = malloc(ninputs * sizeof(EmitModule));
    EmitModule **mod_ptrs = malloc(ninputs * sizeof(EmitModule *));
    for (int i = 0; i < ninputs; i++) {
        size_t len = strlen(inputs[i]);
        if (len >= 2 && inputs[i][len - 2] == '.' && inputs[i][len - 1] == 'o') {
            if (emit_obj_read(inputs[i], &mods[i]) != 0) exit(1);
        } else {
            char *src = read_file(inputs[i]);
            compile_source(src, inputs[i], &mods[i]);
            free(src);
        }
        mod_ptrs[i] = &mods[i];
    }

    emit_link(mod_ptrs, ninputs, output_path,
              (const char **)needed, (size_t)num_needed, nodefaultlibs,
              (const char **)lib_paths, (size_t)num_lib_paths);

    for (int i = 0; i < ninputs; i++) module_free(&mods[i]);
    free(mod_ptrs);
    free(mods);
    free(inputs);
    for (int i = 0; i < num_needed; i++) free(needed[i]);
    free(needed);
    for (int i = 0; i < num_lib_paths; i++) free(lib_paths[i]);
    free(lib_paths);
    return 0;
}
