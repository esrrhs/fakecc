#include "fakecc/lexer.h"
#include "fakecc/common.h"
#include "fakecc/token.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* TokenArray helpers                                                  */
/* ------------------------------------------------------------------ */

void token_array_init(TokenArray *a) {
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

void token_array_free(TokenArray *a) {
    for (size_t i = 0; i < a->len; i++) {
        free(a->data[i].text);
    }
    free(a->data);
    a->data = NULL;
    a->len = 0;
    a->cap = 0;
}

void token_array_push(TokenArray *a, Token t) {
    if (a->len >= a->cap) {
        size_t new_cap = a->cap ? a->cap * 2 : 16;
        a->data = realloc(a->data, new_cap * sizeof(Token));
        if (!a->data) {
            fprintf(stderr, "fakecc: out of memory\n");
            exit(1);
        }
        a->cap = new_cap;
    }
    a->data[a->len++] = t;
}

/* ------------------------------------------------------------------ */
/* Keyword lookup                                                      */
/* ------------------------------------------------------------------ */

static TokenKind keyword_kind(const char *s, size_t len) {
    switch (len) {
    case 2:
        if (memcmp(s, "if", 2) == 0) return TK_KW_INT; /* not a keyword we use, but 'if' is 2 — skip */
        break;
    case 3:
        if (memcmp(s, "int", 3) == 0) return TK_KW_INT;
        break;
    case 6:
        if (memcmp(s, "import", 6) == 0) return TK_KW_IMPORT;
        if (memcmp(s, "return", 6) == 0) return TK_KW_RETURN;
        break;
    case 7:
        if (memcmp(s, "package", 7) == 0) return TK_KW_PACKAGE;
        break;
    default:
        break;
    }
    return TK_IDENT; /* not a keyword */
}

/* ------------------------------------------------------------------ */
/* Lexer implementation                                                */
/* ------------------------------------------------------------------ */

void lex(const char *source, const char *filename, TokenArray *out) {
    size_t pos = 0;
    int line = 1;
    int col = 1;

    /* check if we are at start of a line (for preprocessor detection) */
    int line_start = 1;

    while (source[pos] != '\0') {
        char c = source[pos];

        /* newline */
        if (c == '\n') {
            pos++;
            line++;
            col = 1;
            line_start = 1;
            continue;
        }

        /* other whitespace */
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
            col++;
            continue;
        }

        /* line comment */
        if (c == '/' && source[pos + 1] == '/') {
            pos += 2;
            col += 2;
            while (source[pos] != '\0' && source[pos] != '\n') {
                pos++;
                col++;
            }
            continue;
        }

        /* block comment */
        if (c == '/' && source[pos + 1] == '*') {
            pos += 2;
            col += 2;
            while (source[pos] != '\0') {
                if (source[pos] == '*' && source[pos + 1] == '/') {
                    pos += 2;
                    col += 2;
                    break;
                }
                if (source[pos] == '\n') {
                    pos++;
                    line++;
                    col = 1;
                } else {
                    pos++;
                    col++;
                }
            }
            continue;
        }

        /* preprocessor directive — reject explicitly */
        if (c == '#' && line_start) {
            int start_line = line;
            int start_col = col;
            die_at(filename, start_line, start_col,
                   "preprocessor directives are not supported in FakeCC");
        }

        line_start = 0;

        /* string literal */
        if (c == '"') {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            pos++;  /* skip opening quote */
            col++;
            while (source[pos] != '\0' && source[pos] != '"') {
                if (source[pos] == '\\' && source[pos + 1] != '\0') {
                    pos += 2;
                    col += 2;
                } else if (source[pos] == '\n') {
                    pos++;
                    line++;
                    col = 1;
                } else {
                    pos++;
                    col++;
                }
            }
            if (source[pos] == '"') {
                pos++;
                col++;
            }
            size_t len = pos - start;
            Token t;
            t.kind = TK_STRING_LITERAL;
            t.text = malloc(len + 1);
            memcpy(t.text, source + start, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            continue;
        }

        /* integer literal */
        if (isdigit((unsigned char)c)) {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            while (isdigit((unsigned char)source[pos])) {
                pos++;
                col++;
            }
            size_t len = pos - start;
            Token t;
            t.kind = TK_INT_LITERAL;
            t.text = malloc(len + 1);
            memcpy(t.text, source + start, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            continue;
        }

        /* identifier or keyword */
        if (isalpha((unsigned char)c) || c == '_') {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            while (isalnum((unsigned char)source[pos]) || source[pos] == '_') {
                pos++;
                col++;
            }
            size_t len = pos - start;
            Token t;
            t.kind = keyword_kind(source + start, len);
            t.text = malloc(len + 1);
            memcpy(t.text, source + start, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            continue;
        }

        /* punctuation */
        switch (c) {
        case '(':
        case ')':
        case '{':
        case '}':
        case ';': {
            Token t;
            switch (c) {
            case '(': t.kind = TK_LPAREN; break;
            case ')': t.kind = TK_RPAREN; break;
            case '{': t.kind = TK_LBRACE; break;
            case '}': t.kind = TK_RBRACE; break;
            case ';': t.kind = TK_SEMICOLON; break;
            default:  t.kind = TK_EOF; break; /* unreachable */
            }
            t.text = malloc(2);
            t.text[0] = c;
            t.text[1] = '\0';
            t.loc.file = filename;
            t.loc.line = line;
            t.loc.col = col;
            token_array_push(out, t);
            pos++;
            col++;
            continue;
        }
        default:
            break;
        }

        /* unknown character */
        die_at(filename, line, col, "unexpected character '%c'", c);
    }

    /* emit EOF */
    Token eof;
    eof.kind = TK_EOF;
    eof.text = malloc(1);
    eof.text[0] = '\0';
    eof.loc.file = filename;
    eof.loc.line = line;
    eof.loc.col = col;
    token_array_push(out, eof);
}
