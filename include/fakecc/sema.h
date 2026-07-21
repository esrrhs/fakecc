#ifndef FAKECC_SEMA_H
#define FAKECC_SEMA_H

#include "fakecc/ast.h"

/* Semantic checks for Slice 1. Dies on error. */
void sema_check(const TranslationUnit *tu);

#endif /* FAKECC_SEMA_H */
