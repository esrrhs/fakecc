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

static void usage(void) {
    fprintf(stderr, "usage: fakecc <input.c> -o <output>\n");
    exit(1);
}

int main(int argc, char **argv) {
    if (argc != 4) {
        usage();
    }

    const char *input_path = argv[1];
    if (strcmp(argv[2], "-o") != 0) {
        usage();
    }
    const char *output_path = argv[3];

    /* 1. Read source file */
    char *source = read_file(input_path);

    /* 2. Lex */
    TokenArray tokens;
    token_array_init(&tokens);
    lex(source, input_path, &tokens);

    /* 3. Parse */
    TranslationUnit tu;
    tu_init(&tu);
    parse(&tokens, &tu);

    /* 4. Semantic check */
    sema_check(&tu);

    /* 5. Generate IR */
    IRModule ir;
    ir_module_init(&ir);
    ir_generate(&tu, &ir);

    /* 5.5. Optimize IR */
    opt(&ir);

    /* 6. Generate machine code */
    EmitModule em;
    emit_module_init(&em);
    codegen(&ir, &em);

    /* 7. Write ELF executable */
    emit_elf(&em, output_path);

    /* Cleanup */
    emit_module_free(&em);
    ir_module_free(&ir);
    tu_free(&tu);
    token_array_free(&tokens);
    free(source);

    return 0;
}
