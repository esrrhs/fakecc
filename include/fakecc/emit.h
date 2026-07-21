#ifndef FAKECC_EMIT_H
#define FAKECC_EMIT_H

#include "fakecc/common.h"
#include <stddef.h>

typedef struct {
    char *name;       /* function name, xstrdup'd */
    size_t offset;    /* byte offset in code buffer */
    size_t size;      /* byte size of this function's code */
} EmitSymbol;

typedef struct {
    Buffer code;             /* machine code bytes (all functions concatenated) */
    EmitSymbol *symbols;     /* symbol table */
    size_t num_symbols;
    size_t cap_symbols;
} EmitModule;

void emit_module_init(EmitModule *m);
void emit_module_free(EmitModule *m);
void emit_module_add_symbol(EmitModule *m, const char *name, size_t offset, size_t size);

/* Write a static ELF executable to output_path */
void emit_elf(const EmitModule *m, const char *output_path);

#endif /* FAKECC_EMIT_H */
