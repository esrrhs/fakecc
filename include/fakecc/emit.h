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

/* PLT GOT fixup: patch a rel32 field (a `jmp *GOT(%rip)` in a PLT entry)
 * in the code buffer to got_base_vaddr + got_slot*8.  Emitted by codegen,
 * resolved by emit_elf once the .got.plt vaddr is known. */
typedef struct {
    size_t patch_off;  /* offset of the rel32 field in the code buffer */
    size_t got_slot;   /* GOT entry index (1, 2 for plt0; 3+i for entry i) */
} PltGOTFixup;

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
    /* External (dynamically-linked) functions referenced by this module.
     * Ordered; index i is the relocation index, the PLT slot, and GOT
     * slot 3+i.  Populated by codegen, consumed by emit_elf. */
    char   **ext_names;
    size_t   num_ext;
    size_t   cap_ext;
    size_t  *plt_entry_off;  /* code-buffer offset of each ext's PLT entry */
    PltGOTFixup *plt_got_fixups; /* `jmp *GOT(%rip)` rel32 fields to patch */
    size_t   num_plt_got_fixups;
    size_t   cap_plt_got_fixups;
} EmitModule;

void emit_module_init(EmitModule *m);
void emit_module_free(EmitModule *m);
void emit_module_add_symbol(EmitModule *m, const char *name, size_t offset, size_t size);
void emit_module_add_global(EmitModule *m, const char *name, size_t offset, size_t size, int is_readonly);
void emit_module_add_reloc(EmitModule *m, size_t patch_off, const char *target_name);
/* Record an external function reference (dedup'd; returns its index). */
size_t emit_module_add_external(EmitModule *m, const char *name);
/* Record a PLT entry's code offset for external index `idx`. */
void emit_module_set_plt_entry(EmitModule *m, size_t idx, size_t code_off);
/* Record a PLT `jmp *GOT(%rip)` rel32 field to be patched by emit_elf. */
void emit_module_add_plt_got_fixup(EmitModule *m, size_t patch_off, size_t got_slot);

/* Write a static ELF executable (or, if external calls exist, a
 * dynamically-linked ELF) to output_path. */
void emit_elf(const EmitModule *m, const char *output_path);

#endif /* FAKECC_EMIT_H */
