#ifndef FAKECC_CODEGEN_H
#define FAKECC_CODEGEN_H

#include "fakecc/emit.h"
#include "fakecc/ir.h"

/* Generate x86-64 machine code from IR into out. */
void codegen(const IRModule *ir, EmitModule *out);

#endif /* FAKECC_CODEGEN_H */
