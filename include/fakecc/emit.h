#ifndef FAKECC_EMIT_H
#define FAKECC_EMIT_H

#include "fakecc/common.h"
#include <stddef.h>

typedef struct {
    char *name;       /* function name, xstrdup'd */
    size_t offset;    /* byte offset in code buffer */
    size_t size;      /* byte size of this function's code */
} EmitSymbol;

/* Module-level global variable placed in the data segment. */
typedef struct {
    char *name;             /* xstrdup'd */
    size_t offset;          /* byte offset in the data Buffer */
    size_t size;             /* bytes reserved */
    int    is_readonly;     /* 1 = rodata (currently placed in same RW segment) */
} EmitGlobal;

/* Fixup: patch a 32-bit rel field at `patch_off` in the code buffer to
 * `target_vaddr - (code_vaddr + patch_off + 4)`.  Emitted by IR_GADDR. */
typedef struct {
    size_t patch_off;
    char  *target_name;   /* xstrdup'd */
} EmitGlobalReloc;

typedef struct {
    Buffer code;             /* machine code bytes (all functions concatenated) */
    Buffer data;             /* mutable + rodata globals concatenated */
    EmitSymbol *symbols;     /* function symbol table */
    size_t num_symbols;
    size_t cap_symbols;
    EmitGlobal *globals;     /* data-segment symbol table */
    size_t num_globals;
    size_t cap_globals;
    EmitGlobalReloc *relocs; /* fixups from IR_GADDR / string-literal use */
    size_t num_relocs;
    size_t cap_relocs;
} EmitModule;

void emit_module_init(EmitModule *m);
void emit_module_free(EmitModule *m);
void emit_module_add_symbol(EmitModule *m, const char *name, size_t offset, size_t size);
void emit_module_add_global(EmitModule *m, const char *name, size_t offset, size_t size, int is_readonly);
void emit_module_add_reloc(EmitModule *m, size_t patch_off, const char *target_name);

/* Write a static ELF executable to output_path */
void emit_elf(const EmitModule *m, const char *output_path);

#endif /* FAKECC_EMIT_H */
