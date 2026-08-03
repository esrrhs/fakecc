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
    TK_KW_GOTO,          /* "goto" */
    TK_KW_SWITCH,        /* "switch" */
    TK_KW_CASE,          /* "case" */
    TK_KW_DEFAULT,       /* "default" */
    TK_KW_BREAK,         /* "break" */
    TK_KW_CONTINUE,      /* "continue" */
    TK_KW_CONST,         /* "const" */
    TK_KW_UNION,         /* "union" */
    TK_KW_DO,            /* "do" */
    TK_KW_ENUM,          /* "enum" */
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
    TK_AMP,              /* & — address-of (unary) or bitwise AND (binary) */
    TK_ANDAND,           /* && */
    TK_OROR,             /* || */
    TK_BITOR,            /* | — bitwise OR (binary) */
    TK_XOR,              /* ^ — bitwise XOR (binary) */
    TK_TILDE,            /* ~ — bitwise NOT (unary) */
    TK_NOT,              /* ! — logical NOT (unary) */
    TK_SHL,              /* << — left shift (binary) */
    TK_SHR,              /* >> — right shift (binary) */
    TK_SHL_EQ,           /* <<= */
    TK_SHR_EQ,           /* >>= */
    TK_PLUS_EQ,          /* += */
    TK_MINUS_EQ,         /* -= */
    TK_STAR_EQ,          /* *= */
    TK_SLASH_EQ,         /* /= */
    TK_PERCENT_EQ,       /* %= */
    TK_AMP_EQ,           /* &= */
    TK_BITOR_EQ,         /* |= */
    TK_XOR_EQ,           /* ^= */
    TK_INC,              /* ++ — increment */
    TK_DEC,              /* -- — decrement */
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
