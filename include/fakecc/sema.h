#ifndef FAKECC_SEMA_H
#define FAKECC_SEMA_H

#include "fakecc/ast.h"

/* Semantic checks for Slice 1.  If require_main is non-zero, the TU must
 * define a `main` function (single-file mode).  When compiling one TU of a
 * multi-file program, pass 0 and let the linker verify main globally. */
void sema_check(const TranslationUnit *tu, int require_main);

#endif /* FAKECC_SEMA_H */
