#ifndef FAKECC_CODEGEN_H
#define FAKECC_CODEGEN_H

#include "fakecc/ir.h"

/* Generate AT&T syntax x86-64 assembly from IR into out (appended). */
void codegen(const IRModule *ir, Buffer *out);

#endif /* FAKECC_CODEGEN_H */
