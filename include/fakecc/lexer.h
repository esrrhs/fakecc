#ifndef FAKECC_LEXER_H
#define FAKECC_LEXER_H

#include "fakecc/token.h"

/* Tokenize the entire source string into out.
 * Returns FAKECC_OK on success, FAKECC_ERR on error (message recorded). */
int lex(const char *source, const char *filename, TokenArray *out);

#endif /* FAKECC_LEXER_H */
