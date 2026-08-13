#ifndef FAKECC_DEBUG_H
#define FAKECC_DEBUG_H

#include "fakecc/emit.h"
#include <stddef.h>
#include <stdint.h>

/* Build DWARF4 debug sections from EmitModule debug records.
 * `text_base_vaddr` is the virtual address corresponding to offset 0 of
 * this module's .text. */
void debug_emit_dwarf(const EmitModule *m, uint64_t text_base_vaddr,
                      Buffer *debug_abbrev, Buffer *debug_info,
                      Buffer *debug_str, Buffer *debug_line,
                      Buffer *debug_frame, Buffer *debug_loc);

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
void emit_module_add_dbg_var(EmitModule *m, int func_idx, const DebugVar *v);
void emit_module_add_dbg_global(EmitModule *m, const DebugVar *v);
void emit_module_add_dbg_call_site(EmitModule *m, int func_idx,
                                   const DebugCallSite *cs);

/* Free everything a DebugVar owns and reset it to empty. */
void debug_var_release(DebugVar *v);
void debug_call_site_release(DebugCallSite *cs);

#endif /* FAKECC_DEBUG_H */
