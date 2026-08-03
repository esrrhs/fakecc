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
        if (memcmp(s, "if", 2) == 0) return TK_KW_IF;
        if (memcmp(s, "do", 2) == 0) return TK_KW_DO;
        break;
    case 3:
        if (memcmp(s, "int", 3) == 0) return TK_KW_INT;
        if (memcmp(s, "for", 3) == 0) return TK_KW_FOR;
        break;
    case 4:
        if (memcmp(s, "else", 4) == 0) return TK_KW_ELSE;
        if (memcmp(s, "char", 4) == 0) return TK_KW_CHAR;
        if (memcmp(s, "long", 4) == 0) return TK_KW_LONG;
        if (memcmp(s, "enum", 4) == 0) return TK_KW_ENUM;
        if (memcmp(s, "goto", 4) == 0) return TK_KW_GOTO;
        if (memcmp(s, "case", 4) == 0) return TK_KW_CASE;
        break;
    case 5:
        if (memcmp(s, "while", 5) == 0) return TK_KW_WHILE;
        if (memcmp(s, "short", 5) == 0) return TK_KW_SHORT;
        if (memcmp(s, "break", 5) == 0) return TK_KW_BREAK;
        if (memcmp(s, "union", 5) == 0) return TK_KW_UNION;
        if (memcmp(s, "const", 5) == 0) return TK_KW_CONST;
        break;
    case 6:
        if (memcmp(s, "import", 6) == 0) return TK_KW_IMPORT;
        if (memcmp(s, "return", 6) == 0) return TK_KW_RETURN;
        if (memcmp(s, "signed", 6) == 0) return TK_KW_SIGNED;
        if (memcmp(s, "sizeof", 6) == 0) return TK_KW_SIZEOF;
        if (memcmp(s, "struct", 6) == 0) return TK_KW_STRUCT;
        if (memcmp(s, "switch", 6) == 0) return TK_KW_SWITCH;
        break;
    case 7:
        if (memcmp(s, "package", 7) == 0) return TK_KW_PACKAGE;
        if (memcmp(s, "default", 7) == 0) return TK_KW_DEFAULT;
        break;
    case 8:
        if (memcmp(s, "unsigned", 8) == 0) return TK_KW_UNSIGNED;
        if (memcmp(s, "continue", 8) == 0) return TK_KW_CONTINUE;
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

        /* character literal: 'A', '\n', '\\', '\'', '\0' */
        if (c == '\'') {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            pos++;  /* skip opening quote */
            col++;

            /* must have at least one char (body or escape) */
            if (source[pos] == '\0' || source[pos] == '\n') {
                die_at(filename, start_line, start_col,
                       "unterminated character literal");
            }

            /* escape sequence */
            if (source[pos] == '\\') {
                pos++; col++;  /* skip backslash */
                if (source[pos] == '\0' || source[pos] == '\n') {
                    die_at(filename, start_line, start_col,
                           "unterminated character literal");
                }
                pos++; col++;  /* skip escaped char */
            } else {
                pos++; col++;  /* skip single char */
            }

            /* closing quote */
            if (source[pos] != '\'') {
                die_at(filename, start_line, start_col,
                       "missing closing quote in character literal");
            }
            pos++; col++;  /* skip closing quote */

            size_t len = pos - start;
            Token t;
            t.kind = TK_CHAR_LITERAL;
            t.text = malloc(len + 1);
            memcpy(t.text, source + start, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            continue;
        }

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

        /* two-char operators & comparisons */
        if (c == '=' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_EQ;
            t.text = xstrdup("==");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '+' && source[pos + 1] == '+') {
            Token t;
            t.kind = TK_INC;
            t.text = xstrdup("++");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '-' && source[pos + 1] == '-') {
            Token t;
            t.kind = TK_DEC;
            t.text = xstrdup("--");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '-' && source[pos + 1] == '>') {
            Token t;
            t.kind = TK_ARROW;
            t.text = xstrdup("->");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '!' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_NE;
            t.text = xstrdup("!=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '&' && source[pos + 1] == '&') {
            Token t;
            t.kind = TK_ANDAND;
            t.text = xstrdup("&&");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '|' && source[pos + 1] == '|') {
            Token t;
            t.kind = TK_OROR;
            t.text = xstrdup("||");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '+' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_PLUS_EQ;
            t.text = xstrdup("+=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '-' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_MINUS_EQ;
            t.text = xstrdup("-=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '*' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_STAR_EQ;
            t.text = xstrdup("*=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '/' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_SLASH_EQ;
            t.text = xstrdup("/=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '%' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_PERCENT_EQ;
            t.text = xstrdup("%=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '&' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_AMP_EQ;
            t.text = xstrdup("&=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '|' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_BITOR_EQ;
            t.text = xstrdup("|=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '^' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_XOR_EQ;
            t.text = xstrdup("^=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '<' && source[pos + 1] == '<' && source[pos + 2] == '=') {
            Token t;
            t.kind = TK_SHL_EQ;
            t.text = xstrdup("<<=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 3; col += 3;
            continue;
        }
        if (c == '>' && source[pos + 1] == '>' && source[pos + 2] == '=') {
            Token t;
            t.kind = TK_SHR_EQ;
            t.text = xstrdup(">>=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 3; col += 3;
            continue;
        }
        if (c == '<' && source[pos + 1] == '<') {
            Token t;
            t.kind = TK_SHL;
            t.text = xstrdup("<<");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '>' && source[pos + 1] == '>') {
            Token t;
            t.kind = TK_SHR;
            t.text = xstrdup(">>");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            continue;
        }
        if (c == '<') {
            Token t;
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            if (source[pos + 1] == '=') {
                t.kind = TK_LE;
                t.text = xstrdup("<=");
                pos += 2; col += 2;
            } else {
                t.kind = TK_LT;
                t.text = xstrdup("<");
                pos++; col++;
            }
            token_array_push(out, t);
            continue;
        }
        if (c == '>') {
            Token t;
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            if (source[pos + 1] == '=') {
                t.kind = TK_GE;
                t.text = xstrdup(">=");
                pos += 2; col += 2;
            } else {
                t.kind = TK_GT;
                t.text = xstrdup(">");
                pos++; col++;
            }
            token_array_push(out, t);
            continue;
        }

        /* punctuation */
        switch (c) {
        case '(':
        case ')':
        case '{':
        case '}':
        case '[':
        case ']':
        case ';':
        case ',':
        case '.':
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '&':
        case '|':
        case '^':
        case '~':
        case '=':
        case '!':
        case '?':
        case ':': {
            Token t;
            switch (c) {
            case '(': t.kind = TK_LPAREN; break;
            case ')': t.kind = TK_RPAREN; break;
            case '{': t.kind = TK_LBRACE; break;
            case '}': t.kind = TK_RBRACE; break;
            case '[': t.kind = TK_LBRACKET; break;
            case ']': t.kind = TK_RBRACKET; break;
            case ';': t.kind = TK_SEMICOLON; break;
            case ',': t.kind = TK_COMMA; break;
            case '.': t.kind = TK_DOT; break;
            case '+': t.kind = TK_PLUS; break;
            case '-': t.kind = TK_MINUS; break;
            case '*': t.kind = TK_STAR; break;
            case '/': t.kind = TK_SLASH; break;
            case '%': t.kind = TK_PERCENT; break;
            case '&': t.kind = TK_AMP; break;
            case '|': t.kind = TK_BITOR; break;
            case '^': t.kind = TK_XOR; break;
            case '~': t.kind = TK_TILDE; break;
            case '!': t.kind = TK_NOT; break;
            case '=': t.kind = TK_ASSIGN; break;
            case '?': t.kind = TK_QUESTION; break;
            case ':': t.kind = TK_COLON; break;
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
