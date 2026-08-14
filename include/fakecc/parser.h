#ifndef FAKECC_PARSER_H
#define FAKECC_PARSER_H

#include "fakecc/ast.h"
#include "fakecc/token.h"

struct PkgContext;

/* Parse tokens into a TranslationUnit. Dies on error.
 * `parse` is the no-package entry point (unit tests); `parse_in_pkg` resolves
 * `import` declarations via `ctx` (NULL ctx rejects imports). */
void parse(const TokenArray *tokens, TranslationUnit *tu);
void parse_in_pkg(const TokenArray *tokens, TranslationUnit *tu,
                  struct PkgContext *ctx);

#endif /* FAKECC_PARSER_H */
