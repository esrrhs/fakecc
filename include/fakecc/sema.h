#ifndef FAKECC_SEMA_H
#define FAKECC_SEMA_H

#include "fakecc/ast.h"

struct PkgContext;

/* Semantic checks.  If require_main is non-zero, the TU must define a `main`
 * function (single-file mode).  When compiling one TU of a multi-file
 * program, pass 0 and let the linker verify main globally.
 * `sema_check_in_pkg` resolves qualified names and same-package fallbacks
 * via `ctx` (may be NULL). */
void sema_check(const TranslationUnit *tu, int require_main);
void sema_check_in_pkg(const TranslationUnit *tu, int require_main,
                       struct PkgContext *ctx);

#endif /* FAKECC_SEMA_H */
