#ifndef FAKECC_LEXER_H
#define FAKECC_LEXER_H

#include "fakecc/token.h"

/* Tokenize the entire source string into out. Dies on error. */
void lex(const char *source, const char *filename, TokenArray *out);

#endif /* FAKECC_LEXER_H */
