#ifndef FAKECC_CODEGEN_H
#define FAKECC_CODEGEN_H

#include "fakecc/ast.h"
#include "fakecc/common.h"

/* Generate AT&T syntax x86-64 assembly into out (appended). */
void codegen(const TranslationUnit *tu, Buffer *out);

#endif /* FAKECC_CODEGEN_H */
