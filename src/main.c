#include "fakecc/codegen.h"
#include "fakecc/common.h"
#include "fakecc/lexer.h"
#include "fakecc/parser.h"
#include "fakecc/sema.h"
#include "fakecc/token.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

    /* 5. Codegen */
    Buffer asm_buf;
    buffer_init(&asm_buf);
    codegen(&tu, &asm_buf);

    /* 6. Write temporary .s file */
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/fakecc_%d.s", (int)getpid());

    FILE *asm_file = fopen(tmp_path, "w");
    if (!asm_file) {
        fprintf(stderr, "fakecc: cannot write '%s'\n", tmp_path);
        exit(1);
    }
    fwrite(asm_buf.data, 1, asm_buf.len, asm_file);
    fclose(asm_file);

    /* 7. Call gcc to assemble + link */
    char cmd[512];
    snprintf(cmd, sizeof(cmd), "gcc -x assembler '%s' -o '%s'", tmp_path, output_path);
    int rc = system(cmd);
    if (rc != 0) {
        fprintf(stderr, "fakecc: assembler/linker failed\n");
        unlink(tmp_path);
        exit(1);
    }

    /* 8. Delete temporary file */
    unlink(tmp_path);

    /* Cleanup */
    buffer_free(&asm_buf);
    tu_free(&tu);
    token_array_free(&tokens);
    free(source);

    return 0;
}
