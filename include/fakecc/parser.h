#ifndef FAKECC_PARSER_H
#define FAKECC_PARSER_H

#include "fakecc/ast.h"
#include "fakecc/token.h"

struct PkgContext;

/* Parse tokens into a TranslationUnit.
 * Returns FAKECC_OK on success, FAKECC_ERR on error (message recorded).
 * `parse` is the no-package entry point (unit tests); `parse_in_pkg` resolves
 * `import` declarations via `ctx` (NULL ctx rejects imports). */
int parse(const TokenArray *tokens, TranslationUnit *tu);
int parse_in_pkg(const TokenArray *tokens, TranslationUnit *tu,
                 struct PkgContext *ctx);

#endif /* FAKECC_PARSER_H */
