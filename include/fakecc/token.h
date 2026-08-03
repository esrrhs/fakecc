#ifndef FAKECC_TOKEN_H
#define FAKECC_TOKEN_H

#include "fakecc/common.h"

typedef enum {
    TK_KW_PACKAGE,       /* "package" */
    TK_KW_IMPORT,        /* "import" — Slice 1: recognized but not allowed */
    TK_KW_INT,           /* "int" */
    TK_KW_CHAR,          /* "char" */
    TK_KW_SHORT,         /* "short" */
    TK_KW_LONG,          /* "long" */
    TK_KW_SIGNED,        /* "signed" */
    TK_KW_UNSIGNED,      /* "unsigned" */
    TK_KW_SIZEOF,        /* "sizeof" */
    TK_KW_RETURN,        /* "return" */
    TK_KW_IF,            /* "if" */
    TK_KW_ELSE,          /* "else" */
    TK_KW_WHILE,         /* "while" */
    TK_KW_FOR,           /* "for" */
    TK_KW_BREAK,         /* "break" */
    TK_KW_CONTINUE,      /* "continue" */
    TK_KW_STRUCT,        /* "struct" */
    TK_IDENT,
    TK_INT_LITERAL,
    TK_CHAR_LITERAL,     /* 'A', '\n', etc. */
    TK_STRING_LITERAL,   /* Slice 1: reserved for import, not consumed */
    TK_LPAREN,           /* ( */
    TK_RPAREN,           /* ) */
    TK_LBRACE,           /* { */
    TK_RBRACE,           /* } */
    TK_LBRACKET,         /* [ */
    TK_RBRACKET,         /* ] */
    TK_SEMICOLON,        /* ; */
    TK_COMMA,            /* , */
    TK_PLUS,             /* + */
    TK_MINUS,            /* - */
    TK_STAR,             /* * */
    TK_SLASH,            /* / */
    TK_PERCENT,          /* % */
    TK_AMP,              /* & */
    TK_ANDAND,           /* && */
    TK_OROR,             /* || */
    TK_QUESTION,         /* ? */
    TK_COLON,            /* : */
    TK_DOT,              /* . */
    TK_ARROW,            /* -> */
    TK_ASSIGN,           /* = */
    TK_EQ,               /* == */
    TK_NE,               /* != */
    TK_LT,               /* < */
    TK_LE,               /* <= */
    TK_GT,               /* > */
    TK_GE,               /* >= */
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
