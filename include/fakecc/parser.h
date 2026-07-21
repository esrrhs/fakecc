#ifndef FAKECC_PARSER_H
#define FAKECC_PARSER_H

#include "fakecc/ast.h"
#include "fakecc/token.h"

/* Parse tokens into a TranslationUnit. Dies on error. */
void parse(const TokenArray *tokens, TranslationUnit *tu);

#endif /* FAKECC_PARSER_H */
