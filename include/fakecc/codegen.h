#ifndef FAKECC_CODEGEN_H
#define FAKECC_CODEGEN_H

#include "fakecc/emit.h"
#include "fakecc/ir.h"

/* Generate x86-64 machine code from IR into out.
 * When `want_debug` is non-zero, records PC↔line mappings and variable
 * locations on `out` for later DWARF emission. */
void codegen(const IRModule *ir, EmitModule *out, int want_debug);

#endif /* FAKECC_CODEGEN_H */
