#ifndef FAKECC_TOKEN_H
#define FAKECC_TOKEN_H

#include "fakecc/common.h"

typedef enum {
    TK_KW_PACKAGE,       /* "package" */
    TK_KW_IMPORT,        /* "import" — Slice 1: recognized but not allowed */
    TK_KW_INT,           /* "int" */
    TK_KW_RETURN,        /* "return" */
    TK_IDENT,
    TK_INT_LITERAL,
    TK_STRING_LITERAL,   /* Slice 1: reserved for import, not consumed */
    TK_LPAREN,           /* ( */
    TK_RPAREN,           /* ) */
    TK_LBRACE,           /* { */
    TK_RBRACE,           /* } */
    TK_SEMICOLON,        /* ; */
    TK_EOF,
} TokenKind;

typedef struct {
    TokenKind kind;
    char *text;          /* strdup'd original text; owner = TokenArray */
    SourceLoc loc;
} Token;

typedef struct {
    Token *data;
    size_t len;
    size_t cap;
} TokenArray;

void token_array_init(TokenArray *a);
void token_array_free(TokenArray *a);
void token_array_push(TokenArray *a, Token t);

#endif /* FAKECC_TOKEN_H */
