#ifndef FAKECC_EMIT_H
#define FAKECC_EMIT_H

#include "fakecc/common.h"
#include <stddef.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* Section indices (also used as ELF st_shndx for defined symbols)      */
/* ------------------------------------------------------------------ */

#define SECT_UNDEF  0
#define SECT_TEXT   1
#define SECT_RODATA 2
#define SECT_DATA   3
#define SECT_BSS    4

/* ------------------------------------------------------------------ */
/* Symbol table entry                                                  */
/* ------------------------------------------------------------------ */

typedef struct {
    char *name;        /* symbol name (NULL for section symbols) */
    uint8_t binding;   /* STB_LOCAL (0) / STB_GLOBAL (1) */
    uint8_t type;      /* STT_NOTYPE(0) / STT_OBJECT(1) / STT_FUNC(2) / STT_SECTION(3) */
    uint16_t shndx;    /* section index, or SHN_UNDEF(0) for undefined */
    size_t value;      /* offset within section (defined symbols) */
    size_t size;       /* byte size (0 for undefined symbols) */
} EmitSymbol;

/* ------------------------------------------------------------------ */
/* Relocation entry                                                    */
/* ------------------------------------------------------------------ */

typedef struct {
    size_t   offset;   /* offset within the section being relocated */
    uint32_t type;     /* R_X86_64_PC32(2), R_X86_64_32(1), etc. */
    uint32_t sym;      /* target symbol index into syms[] */
    int32_t  addend;   /* addend (rip-relative uses -4) */
} EmitReloc;

/* ------------------------------------------------------------------ */
/* Object module — the in-memory representation of one compiled TU     */
/* ------------------------------------------------------------------ */

typedef struct {
    Buffer   text;     /* .text */
    Buffer   rodata;   /* .rodata (string literals, long double constants) */
    Buffer   data;     /* .data (mutable globals) */
    size_t   bss_size; /* .bss total bytes (zero-initialized globals) */

    EmitSymbol *syms;  /* unified symbol table (section + defined + undefined) */
    size_t num_syms, cap_syms;

    EmitReloc  *relocs; /* relocations (all within .text) */
    size_t num_relocs, cap_relocs;
} EmitModule;

/* ------------------------------------------------------------------ */
/* Lifetime                                                            */
/* ------------------------------------------------------------------ */

void emit_module_init(EmitModule *m);
void emit_module_free(EmitModule *m);

/* ------------------------------------------------------------------ */
/* Symbol table                                                        */
/* ------------------------------------------------------------------ */

/* Add a defined symbol; returns its index. */
int  emit_module_add_symbol(EmitModule *m, const char *name,
                            uint8_t binding, uint8_t type,
                            uint16_t shndx, size_t value, size_t size);
/* Find a defined symbol by name; returns index or -1. */
int  emit_module_find_symbol(EmitModule *m, const char *name);
/* Add (or reuse) an undefined symbol; returns its index. */
int  emit_module_add_undefined(EmitModule *m, const char *name);

/* ------------------------------------------------------------------ */
/* Relocations                                                         */
/* ------------------------------------------------------------------ */

void emit_module_add_reloc(EmitModule *m, size_t offset, uint32_t type,
                           int sym, int32_t addend);

/* ------------------------------------------------------------------ */
/* Output                                                              */
/* ------------------------------------------------------------------ */

/* Write a relocatable object file (ET_REL) to path. */
void emit_obj(const EmitModule *m, const char *path);
/* Read a relocatable object file into an EmitModule.  Returns 0 on success. */
int  emit_obj_read(const char *path, EmitModule *m);

/* Link one or more object modules into an executable.  If any module
 * references undefined symbols, a dynamically-linked executable with a
 * PLT/GOT for libc is produced; otherwise a static executable. */
void emit_link(EmitModule **mods, size_t n, const char *path);

/* Legacy entry point — compile a single TU directly into an executable.
 * Equivalent to link(&m, 1, path). */
void emit_elf(const EmitModule *m, const char *path);

/* ELF relocation type constants. */
#define R_X86_64_32        1
#define R_X86_64_PC32      2
#define R_X86_64_PLT32     4
#define R_X86_64_GOTPCREL  9
#define R_X86_64_GLOB_DAT  6

#endif /* FAKECC_EMIT_H */
