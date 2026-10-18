#ifndef FAKECC_DEBUG_H
#define FAKECC_DEBUG_H

#include "fakecc/emit.h"
#include <stddef.h>
#include <stdint.h>

/* TypeKind -> the DWARF type tag a debug record carries.  Shared by both
 * backends, since each turns the same TypeKinds into the same tags. */
DebugTypeTag ir_type_tag(int type_kind, int is_bool);

/* Build DWARF4 debug sections from EmitModule debug records.
 * `text_base_vaddr` is the virtual address corresponding to offset 0 of
 * this module's .text.
 *
 * `sec_vaddr`, when non-NULL, is indexed by SECT_* and gives the virtual
 * address each section starts at.  m->syms[].value is a *section offset*
 * for a module that has not been laid out yet, and DW_OP_addr has to be an
 * absolute address, so a backend that still holds offsets passes the bases
 * here.  The ELF linker has already rebased its symbols onto the linked
 * image and passes NULL. */
void debug_emit_dwarf(const EmitModule *m, uint64_t text_base_vaddr,
                      Buffer *debug_abbrev, Buffer *debug_info,
                      Buffer *debug_str, Buffer *debug_line,
                      Buffer *debug_frame, Buffer *debug_loc,
                      const uint64_t *sec_vaddr);

/* Serialize / deserialize .fakecc_dbg payload (structured records). */
void debug_serialize(const EmitModule *m, Buffer *out);
int  debug_deserialize(EmitModule *m, const unsigned char *data, size_t len);

/* Helpers used by codegen / IR. */
void emit_module_add_dbg_line(EmitModule *m, const char *file, int line,
                              int col, size_t pc_off);
int  emit_module_add_dbg_func(EmitModule *m, const char *name,
                              const char *file, int line, size_t start_pc);
void emit_module_dbg_func_end(EmitModule *m, int func_idx, size_t end_pc,
                              size_t prologue_end_pc);
/* Record where `push %rbp` / `mov %rsp,%rbp` finish, for step-by-step CFI. */
void emit_module_dbg_func_frame(EmitModule *m, int func_idx,
                                size_t after_push_rbp_pc,
                                size_t after_mov_rbp_pc);
/* arm64 counterpart: `stp x29,x30,[sp,#-frame_size]!` then `add x29,sp,#0`.
 * The unwinder needs the byte count the prologue pushed -- unlike x86-64 it
 * is not a constant, it grows with the callee-saved registers. */
void emit_module_dbg_func_frame64(EmitModule *m, int func_idx,
                                  size_t after_stp_pc,
                                  size_t after_mov_fp_pc,
                                  size_t frame_size);
void emit_module_add_dbg_var(EmitModule *m, int func_idx, const DebugVar *v);
void emit_module_add_dbg_global(EmitModule *m, const DebugVar *v);
void emit_module_add_dbg_call_site(EmitModule *m, int func_idx,
                                   const DebugCallSite *cs);

/* Free everything a DebugVar owns and reset it to empty. */
void debug_var_release(DebugVar *v);
void debug_call_site_release(DebugCallSite *cs);

#endif /* FAKECC_DEBUG_H */
