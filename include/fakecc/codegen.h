#ifndef FAKECC_CODEGEN_H
#define FAKECC_CODEGEN_H

#include "fakecc/emit.h"
#include "fakecc/ir.h"

/* Generate x86-64 machine code from IR into out.
 * When `want_debug` is non-zero, records PC↔line mappings and variable
 * locations on `out` for later DWARF emission. */
void codegen(const IRModule *ir, EmitModule *out, int want_debug);

/* x86_64 code model (fakeos M4 / -mcmodel=).  0 = small (default), 1 = kernel
 * (negative 2GiB / higher-half).  See docs/elf-et-rel-higher-half.md. */
void codegen_set_code_model(int kernel);
int  codegen_code_model_is_kernel(void);

#endif /* FAKECC_CODEGEN_H */
