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
/* Debug records (populated by codegen when -g; serialized in .o)      */
/* ------------------------------------------------------------------ */

typedef enum {
    DBG_VAR_PARAM  = 0,
    DBG_VAR_LOCAL  = 1,
    DBG_VAR_GLOBAL = 2
} DebugVarKind;

typedef enum {
    DBG_TY_VOID   = 0,
    DBG_TY_INT    = 1,
    DBG_TY_FLOAT  = 2,
    DBG_TY_PTR    = 3,
    DBG_TY_ARRAY  = 4,
    DBG_TY_STRUCT = 5,
    DBG_TY_BOOL   = 6
} DebugTypeTag;

typedef enum {
    DBG_LOC_NONE  = 0,
    DBG_LOC_FBREG = 1,  /* rbp + rbp_offset */
    DBG_LOC_REG   = 2,  /* DWARF register number in dwarf_reg */
    DBG_LOC_ADDR  = 3,  /* absolute address via sym_name (link resolves) */
    /* Value of the ABI arrival register as it was on entry to this function.
     * Resolved by the debugger via DW_TAG_call_site_parameter. */
    DBG_LOC_ENTRY_VALUE = 4
} DebugLocKind;

/* One entry of a DWARF location list: over [pc_start, pc_end) within the
 * module's .text, the variable lives at the described location.  Optimized
 * variables move between registers and spill slots, so a single location
 * expression is not enough — this mirrors gcc's .debug_loc behaviour. */
typedef struct {
    size_t pc_start;
    size_t pc_end;
    DebugLocKind loc_kind;
    int rbp_offset;
    int dwarf_reg;
} DebugLocRange;

/* One field of a DBG_TY_STRUCT, used to emit DW_TAG_member.  Nested structs
 * are represented as DBG_TY_STRUCT with only width (no further members here);
 * that is enough for sizeof / offsetof of the outer type and for `print p.x`
 * when the member itself is a scalar. */
typedef struct {
    char *name;
    int offset;
    int bit_width;      /* 0 = normal member */
    int bit_offset;
    DebugTypeTag type_tag;
    int width;
    int is_unsigned;
} DebugMember;

typedef struct {
    char *name;
    char *file;
    int line;
    DebugVarKind kind;
    DebugTypeTag type_tag;
    int width;          /* byte size of the object */
    int is_unsigned;
    int array_len;      /* DBG_TY_ARRAY element count; else 0 */
    /* Struct tag when type_tag==DBG_TY_STRUCT, or pointee struct tag when
     * type_tag==DBG_TY_PTR and the pointee is a named struct. */
    char *type_name;
    int struct_size;        /* byte size of that struct; 0 if unknown */
    DebugMember *members;
    size_t num_members;
    /* Single-location form: used when the variable has one home for its
     * whole scope (stack slot, or a global's address). */
    DebugLocKind loc_kind;
    int rbp_offset;     /* DBG_LOC_FBREG */
    int dwarf_reg;      /* DBG_LOC_REG (x86-64 DWARF numbering) */
    char *sym_name;     /* DBG_LOC_ADDR: ELF symbol name */
    /* Location-list form: non-empty when the variable moves between homes.
     * Takes precedence over loc_kind when num_ranges > 0. */
    DebugLocRange *ranges;
    size_t num_ranges, cap_ranges;
    /* Scratch for codegen before locations are finalized: */
    int alloca_ssa;     /* local/param alloca SSA id, or -1 */
    int param_idx;      /* SysV param index, or -1 */
    /* For DBG_VAR_PARAM: DWARF register the argument arrived in (SysV), or
     * -1 if it arrived on the stack.  Used to emit DW_OP_entry_value so outer
     * frames can recover the parameter after caller-saved homes are clobbered. */
    int entry_dwarf_reg;
} DebugVar;

/* One register argument at a call site (DW_TAG_call_site_parameter). */
typedef struct {
    int dwarf_reg;              /* DW_AT_location: DW_OP_regN */
    unsigned char *value_expr;  /* DW_AT_call_value expression bytes */
    size_t value_expr_len;
} DebugCallSiteParam;

typedef struct {
    size_t call_pc;             /* offset of the CALL instruction */
    size_t return_pc;           /* offset of the instruction after CALL */
    char *callee_name;          /* direct callee; NULL if indirect */
    DebugCallSiteParam *params;
    size_t num_params;
} DebugCallSite;

typedef struct {
    char *file;
    int line;
    int col;
    size_t pc_off;      /* offset within this module's .text */
} DebugLineEntry;

typedef struct {
    char *name;
    char *file;
    int line;
    size_t start_pc;
    size_t end_pc;
    size_t prologue_end_pc;
    /* Where the frame is established, so call-frame info can describe the
     * prologue step by step.  gdb routinely stops mid-prologue (`break f`
     * skips `push %rbp; mov %rsp,%rbp` on its own), and a single CFI rule for
     * the whole prologue makes it unwind into a bogus frame there. */
    size_t after_push_rbp_pc;   /* just past `push %rbp` */
    size_t after_mov_rbp_pc;    /* just past `mov %rsp,%rbp` */
    DebugVar *vars;
    size_t num_vars, cap_vars;
    DebugCallSite *call_sites;
    size_t num_call_sites, cap_call_sites;
} DebugFunc;

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
    EmitReloc  *data_relocs; /* relocations within .data (pointer fixups) */
    size_t num_data_relocs, cap_data_relocs;

    /* Debug info (-g).  Empty when compiled without -g. */
    char *dbg_tu_name;           /* primary source file name */
    DebugLineEntry *dbg_lines;
    size_t num_dbg_lines, cap_dbg_lines;
    DebugFunc *dbg_funcs;
    size_t num_dbg_funcs, cap_dbg_funcs;
    DebugVar *dbg_globals;
    size_t num_dbg_globals, cap_dbg_globals;
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
void emit_module_add_data_reloc(EmitModule *m, size_t offset, uint32_t type,
                                int sym, int32_t addend);

/* ------------------------------------------------------------------ */
/* Output                                                              */
/* ------------------------------------------------------------------ */

/* Write a relocatable object file (ET_REL) to path. */
void emit_obj(const EmitModule *m, const char *path);
/* Read a relocatable object file into an EmitModule.  Returns 0 on success. */
int  emit_obj_read(const char *path, EmitModule *m);

/* Link one or more object modules into an executable.
 *
 * If any module references undefined symbols, a dynamically-linked executable
 * with a PLT/GOT is produced; otherwise a static executable.
 *
 * `needed` / `num_needed` are DT_NEEDED sonames (e.g. "libc.so.6", "libfoo.so"),
 * typically derived from `-l` flags.  Defaults never auto-add libc — the driver
 * links builtin `rt/` instead (Go-like freestanding).  Pass `-l` for optional
 * dynamic interop.  `nodefaultlibs` is retained as a no-op for compatibility.
 * `needed` may be NULL when `num_needed` is 0.
 *
 * `lib_paths` / `num_lib_paths` come from `-L` (and directories of explicit `.so`
 * inputs).  When non-empty on a dynamic link they are joined with ':' into
 * DT_RUNPATH so the dynamic linker can find non-system libraries without
 * LD_LIBRARY_PATH. */
void emit_link(EmitModule **mods, size_t n, const char *path,
               const char **needed, size_t num_needed, int nodefaultlibs,
               const char **lib_paths, size_t num_lib_paths,
               int want_debug);

/* Legacy entry point — compile a single TU directly into an executable.
 * Equivalent to emit_link(&m, 1, path, NULL, 0, 0, NULL, 0, 0). */
void emit_elf(const EmitModule *m, const char *path);

/* ELF relocation type constants. */
#define R_X86_64_32        1
#define R_X86_64_PC32      2
#define R_X86_64_PLT32     4
#define R_X86_64_GOTPCREL  9
#define R_X86_64_GLOB_DAT  6
#define R_X86_64_64       10  /* absolute 64-bit (pointer fixups in .data) */

#endif /* FAKECC_EMIT_H */
