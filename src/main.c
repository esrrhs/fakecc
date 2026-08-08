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

/* Compile one .c source into an EmitModule.  Caller frees via
 * codegen_module_free (below). */
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
    fprintf(stderr, "usage: fakecc [-c] <input...> -o <output>\n");
    exit(1);
}

int main(int argc, char **argv) {
    int compile_only = 0;
    const char *output_path = NULL;
    const char **inputs = NULL;
    int ninputs = 0;

    /* Parse arguments: collect inputs, detect -c and -o. */
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-c") == 0) {
            compile_only = 1;
        } else if (strcmp(argv[i], "-o") == 0) {
            if (i + 1 >= argc) usage();
            output_path = argv[++i];
        } else {
            inputs = realloc(inputs, (ninputs + 1) * sizeof(char *));
            inputs[ninputs++] = argv[i];
        }
    }

    if (ninputs == 0 || output_path == NULL) usage();

    if (compile_only) {
        /* Compile-only: exactly one input → object file. */
        if (ninputs != 1) {
            fprintf(stderr, "fakecc: -c requires exactly one input\n");
            exit(1);
        }
        EmitModule em;
        char *src = read_file(inputs[0]);
        compile_source(src, inputs[0], &em);
        free(src);
        emit_obj(&em, output_path);
        module_free(&em);
        return 0;
    }

    /* Compile/read each input into an EmitModule, then link. */
    EmitModule *mods = malloc(ninputs * sizeof(EmitModule));
    EmitModule **mod_ptrs = malloc(ninputs * sizeof(EmitModule *));
    for (int i = 0; i < ninputs; i++) {
        size_t len = strlen(inputs[i]);
        if (len >= 2 && inputs[i][len - 2] == '.' && inputs[i][len - 1] == 'o') {
            /* Object file — read it back. */
            if (emit_obj_read(inputs[i], &mods[i]) != 0) exit(1);
        } else {
            char *src = read_file(inputs[i]);
            compile_source(src, inputs[i], &mods[i]);
            free(src);
        }
        mod_ptrs[i] = &mods[i];
    }

    emit_link(mod_ptrs, ninputs, output_path);

    for (int i = 0; i < ninputs; i++) module_free(&mods[i]);
    free(mod_ptrs);
    free(mods);
    free(inputs);
    return 0;
}
