#include "fakecc/lexer.h"
#include "fakecc/common.h"
#include "fakecc/token.h"

#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Predefined macro → literal conversion                              */
/* ------------------------------------------------------------------ */
/* GCC predefined macros (e.g. __INT_MAX__, __CHAR_BIT__) are replaced
 * by the preprocessor before the compiler proper sees them.  FakeCC has
 * no separate preprocessor, so we fold them into literal tokens right
 * here in the lexer — that makes them valid integer-constant-expressions
 * in every context (case labels, array bounds, enum values, static
 * initializers, etc.), exactly as GCC intends.  Returns NULL if `text`
 * is not a recognised predefined macro.  The returned string is a
 * static constant and must not be freed. */
static const char *predefined_macro_literal(const char *text, int *out_is_float) {
    *out_is_float = 0;
    if (strcmp(text, "__INT_MAX__") == 0) return "2147483647";
    if (strcmp(text, "__INT_MIN__") == 0) return "-2147483648";
    if (strcmp(text, "__UINT_MAX__") == 0) return "4294967295u";
    if (strcmp(text, "__LONG_MAX__") == 0) return "9223372036854775807l";
    if (strcmp(text, "__LONG_MIN__") == 0) return "-9223372036854775807l";
    if (strcmp(text, "__ULONG_MAX__") == 0) return "18446744073709551615ul";
    if (strcmp(text, "__CHAR_BIT__") == 0) return "8";
    if (strcmp(text, "__SIZEOF_INT__") == 0) return "4";
    if (strcmp(text, "__SIZEOF_LONG__") == 0) return "8";
    if (strcmp(text, "__SIZEOF_LONG_LONG__") == 0) return "8";
    if (strcmp(text, "__SIZEOF_POINTER__") == 0) return "8";
    if (strcmp(text, "__SIZEOF_FLOAT__") == 0) return "4";
    if (strcmp(text, "__SIZEOF_DOUBLE__") == 0) return "8";
    if (strcmp(text, "__SIZEOF_LONG_DOUBLE__") == 0) return "16";
    if (strcmp(text, "__FLT_MAX__") == 0) { *out_is_float = 1; return "3.40282347e+38f"; }
    if (strcmp(text, "__DBL_MAX__") == 0) { *out_is_float = 1; return "1.7976931348623157e+308"; }
    if (strcmp(text, "__FLT_MIN__") == 0) { *out_is_float = 1; return "1.17549435e-38f"; }
    if (strcmp(text, "__DBL_MIN__") == 0) { *out_is_float = 1; return "2.2250738585072014e-308"; }
    if (strcmp(text, "__FLT_EPSILON__") == 0) { *out_is_float = 1; return "1.19209290e-7f"; }
    if (strcmp(text, "__DBL_EPSILON__") == 0) { *out_is_float = 1; return "2.2204460492503131e-16"; }
    return NULL;
}

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
        if (memcmp(s, "void", 4) == 0) return TK_KW_VOID;
        break;
    case 5:
        if (memcmp(s, "while", 5) == 0) return TK_KW_WHILE;
        if (memcmp(s, "short", 5) == 0) return TK_KW_SHORT;
        if (memcmp(s, "break", 5) == 0) return TK_KW_BREAK;
        if (memcmp(s, "union", 5) == 0) return TK_KW_UNION;
        if (memcmp(s, "const", 5) == 0) return TK_KW_CONST;
        if (memcmp(s, "float", 5) == 0) return TK_KW_FLOAT;
        if (memcmp(s, "_Bool", 5) == 0) return TK_KW_BOOL;
        break;
    case 6:
        if (memcmp(s, "double", 6) == 0) return TK_KW_DOUBLE;
        if (memcmp(s, "import", 6) == 0) return TK_KW_IMPORT;
        if (memcmp(s, "return", 6) == 0) return TK_KW_RETURN;
        if (memcmp(s, "signed", 6) == 0) return TK_KW_SIGNED;
        if (memcmp(s, "sizeof", 6) == 0) return TK_KW_SIZEOF;
        if (memcmp(s, "static", 6) == 0) return TK_KW_STATIC;
        if (memcmp(s, "struct", 6) == 0) return TK_KW_STRUCT;
        if (memcmp(s, "switch", 6) == 0) return TK_KW_SWITCH;
        if (memcmp(s, "extern", 6) == 0) return TK_KW_EXTERN;
        if (memcmp(s, "inline", 6) == 0) return TK_KW_INLINE;
        /* __real / __imag (without trailing __) */
        if (memcmp(s, "__real", 6) == 0) return TK_KW_REAL;
        if (memcmp(s, "__imag", 6) == 0) return TK_KW_IMAG;
        break;
    case 7:
        if (memcmp(s, "package", 7) == 0) return TK_KW_PACKAGE;
        if (memcmp(s, "default", 7) == 0) return TK_KW_DEFAULT;
        if (memcmp(s, "typedef", 7) == 0) return TK_KW_TYPEDEF;
        if (memcmp(s, "alignof", 7) == 0) return TK_KW_ALIGNOF;
        if (memcmp(s, "__const", 7) == 0) return TK_KW_CONST;
        break;
    case 8:
        if (memcmp(s, "unsigned", 8) == 0) return TK_KW_UNSIGNED;
        if (memcmp(s, "continue", 8) == 0) return TK_KW_CONTINUE;
        if (memcmp(s, "volatile", 8) == 0) return TK_KW_VOLATILE;
        if (memcmp(s, "restrict", 8) == 0) return TK_KW_RESTRICT;
        if (memcmp(s, "_Alignof", 8) == 0) return TK_KW_ALIGNOF;
        if (memcmp(s, "_Complex", 8) == 0) return TK_KW_COMPLEX;
        if (memcmp(s, "__real__", 8) == 0) return TK_KW_REAL;
        if (memcmp(s, "__imag__", 8) == 0) return TK_KW_IMAG;
        if (memcmp(s, "__signed", 8) == 0) return TK_KW_SIGNED;
        if (memcmp(s, "__inline", 8) == 0) return TK_KW_INLINE;
        break;
    case 9:
        if (memcmp(s, "__alignof", 9) == 0) return TK_KW_ALIGNOF;
        if (memcmp(s, "__const__", 9) == 0) return TK_KW_CONST;
        if (memcmp(s, "__complex", 9) == 0) return TK_KW_COMPLEX;
        break;
    case 10:
        if (memcmp(s, "__signed__", 10) == 0) return TK_KW_SIGNED;
        if (memcmp(s, "__inline__", 10) == 0) return TK_KW_INLINE;
        if (memcmp(s, "__volatile", 10) == 0) return TK_KW_VOLATILE;
        if (memcmp(s, "__restrict", 10) == 0) return TK_KW_RESTRICT;
        break;
    case 11:
        if (memcmp(s, "__alignof__", 11) == 0) return TK_KW_ALIGNOF;
        if (memcmp(s, "__complex__", 11) == 0) return TK_KW_COMPLEX;
        if (memcmp(s, "__extension", 11) == 0) return TK_KW_INLINE;
        break;
    case 12:
        if (memcmp(s, "__volatile__", 12) == 0) return TK_KW_VOLATILE;
        if (memcmp(s, "__restrict__", 12) == 0) return TK_KW_RESTRICT;
        break;
    case 13:
        if (memcmp(s, "__extension__", 13) == 0) return TK_KW_INLINE;
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
    char c;

lex_loop_head:
    while (source[pos] != '\0') {
        c = source[pos];

        /* newline */
        if (c == '\n') {
            pos++;
            line++;
            col = 1;
            line_start = 1;
            goto lex_loop_head;
        }

        /* other whitespace */
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
            col++;
            goto lex_loop_head;
        }

        /* line comment */
        if (c == '/' && source[pos + 1] == '/') {
            pos += 2;
            col += 2;
            while (source[pos] != '\0' && source[pos] != '\n') {
                pos++;
                col++;
            }
            goto lex_loop_head;
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
            goto lex_loop_head;
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

            while (source[pos] != '\'' && source[pos] != '\0' && source[pos] != '\n') {
                if (source[pos] == '\\') {
                    pos++; col++;
                    if (source[pos] == '\0' || source[pos] == '\n') break;
                    if (source[pos] == 'x' || source[pos] == 'X') {
                        pos++; col++;
                        if (!isxdigit((unsigned char)source[pos]))
                            die_at(filename, start_line, start_col,
                                   "hex escape \\x with no digits");
                        while (isxdigit((unsigned char)source[pos])) { pos++; col++; }
                    } else if (source[pos] >= '0' && source[pos] <= '7') {
                        int n = 0;
                        while (n < 3 && source[pos] >= '0' && source[pos] <= '7') {
                            pos++; col++; n++;
                        }
                    } else {
                        pos++; col++;
                    }
                } else {
                    pos++; col++;
                }
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
            goto lex_loop_head;
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
            goto lex_loop_head;
        }

        /* numeric literal — integer or floating point.
         *   123, 123u, 123l, 123ul  (integer)
         *   1.5, .5, 2., 1e10, 1.5e-3 (floating)
         *   1.5f, 2.f, .5f            (float suffix)
         * A decimal point or exponent marker upgrades the token to floating. */
        if (isdigit((unsigned char)c)) {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            int is_float = 0;
            int is_hex = 0;
            /* Hex literal: `0x` / `0X` followed by hex digits.  Octal needs
             * no special lexing (`077` is consumed as digits and decoded as
             * octal by strtol(base=0)); only hex stops the digit loop early. */
            if (c == '0' && (source[pos + 1] == 'x' || source[pos + 1] == 'X')) {
                is_hex = 1;
                pos += 2;  /* "0x" */
                col += 2;
                if (!isxdigit((unsigned char)source[pos]))
                    die_at(filename, start_line, start_col,
                           "hex literal has no digits");
                while (isxdigit((unsigned char)source[pos])) { pos++; col++; }
                /* Hex float: 0x1.fp1 / 0x1p1 (binary exponent). */
                if (source[pos] == '.') {
                    is_float = 1;
                    pos++; col++;
                    while (isxdigit((unsigned char)source[pos])) { pos++; col++; }
                }
                if (source[pos] == 'p' || source[pos] == 'P') {
                    is_float = 1;
                    pos++; col++;
                    if (source[pos] == '+' || source[pos] == '-') { pos++; col++; }
                    if (!isdigit((unsigned char)source[pos])) {
                        die_at(filename, line, col, "hex float exponent has no digits");
                    }
                    while (isdigit((unsigned char)source[pos])) { pos++; col++; }
                }
            } else {
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            /* Fractional part: `.` followed by optional digits. Also bare `.`
             * after digits (e.g. `2.`).  Not used for hex (already handled). */
            if (!is_hex && source[pos] == '.') {
                is_float = 1;
                pos++; col++;
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            /* Exponent: e/E followed by optional sign and digits. */
            if (!is_hex && (source[pos] == 'e' || source[pos] == 'E')) {
                is_float = 1;
                pos++; col++;
                if (source[pos] == '+' || source[pos] == '-') { pos++; col++; }
                if (!isdigit((unsigned char)source[pos])) {
                    die_at(filename, line, col,
                           "exponent has no digits");
                }
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            /* Suffix (integer / float / imaginary) */
            for (;;) {
                char sc = source[pos];
                if (sc == 'u' || sc == 'U' || sc == 'l' || sc == 'L' ||
                    sc == 'f' || sc == 'F' || sc == 'i' || sc == 'I' ||
                    sc == 'j' || sc == 'J') {
                    if (sc == 'f' || sc == 'F' || sc == 'i' || sc == 'I' || sc == 'j' || sc == 'J')
                        is_float = 1;
                    pos++; col++;
                } else break;
            }
            size_t len = pos - start;
            Token t;
            t.kind = is_float ? TK_FLOAT_LITERAL : TK_INT_LITERAL;
            t.text = malloc(len + 1);
            memcpy(t.text, source + start, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            goto lex_loop_head;
        }

        /* A leading `.` followed by a digit is a floating literal: .5, .123 */
        if (c == '.' && isdigit((unsigned char)source[pos + 1])) {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            pos++; col++;  /* consume `.` */
            while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            if (source[pos] == 'e' || source[pos] == 'E') {
                pos++; col++;
                if (source[pos] == '+' || source[pos] == '-') { pos++; col++; }
                if (!isdigit((unsigned char)source[pos])) {
                    die_at(filename, line, col, "exponent has no digits");
                }
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            for (;;) {
                char sc = source[pos];
                if (sc == 'f' || sc == 'F' || sc == 'l' || sc == 'L' ||
                    sc == 'i' || sc == 'I' || sc == 'j' || sc == 'J') {
                    pos++; col++;
                } else break;
            }
            size_t len = pos - start;
            Token t;
            t.kind = TK_FLOAT_LITERAL;
            t.text = malloc(len + 1);
            memcpy(t.text, source + start, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            goto lex_loop_head;
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
            /* Check for GCC predefined macros — fold them into literal
             * tokens so they are valid constant expressions everywhere.
             * `ident` is not yet null-terminated, so build a temporary
             * null-terminated copy for the lookup. */
            char ident_buf[64];
            memcpy(ident_buf, source + start, len);
            ident_buf[len] = '\0';
            int is_float = 0;
            const char *lit = predefined_macro_literal(ident_buf, &is_float);
            if (lit) {
                Token t;
                t.kind = is_float ? TK_FLOAT_LITERAL : TK_INT_LITERAL;
                t.text = xstrdup(lit);
                t.loc.file = filename;
                t.loc.line = start_line;
                t.loc.col = start_col;
                token_array_push(out, t);
                goto lex_loop_head;
            }
            Token t;
            t.kind = keyword_kind(ident_buf, len);
            t.text = malloc(len + 1);
            memcpy(t.text, ident_buf, len);
            t.text[len] = '\0';
            t.loc.file = filename;
            t.loc.line = start_line;
            t.loc.col = start_col;
            token_array_push(out, t);
            goto lex_loop_head;
        }

        /* two-char operators & comparisons */
        if (c == '=' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_EQ;
            t.text = xstrdup("==");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '+' && source[pos + 1] == '+') {
            Token t;
            t.kind = TK_INC;
            t.text = xstrdup("++");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '-' && source[pos + 1] == '-') {
            Token t;
            t.kind = TK_DEC;
            t.text = xstrdup("--");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '-' && source[pos + 1] == '>') {
            Token t;
            t.kind = TK_ARROW;
            t.text = xstrdup("->");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '!' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_NE;
            t.text = xstrdup("!=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '&' && source[pos + 1] == '&') {
            Token t;
            t.kind = TK_ANDAND;
            t.text = xstrdup("&&");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '|' && source[pos + 1] == '|') {
            Token t;
            t.kind = TK_OROR;
            t.text = xstrdup("||");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '+' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_PLUS_EQ;
            t.text = xstrdup("+=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '-' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_MINUS_EQ;
            t.text = xstrdup("-=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '*' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_STAR_EQ;
            t.text = xstrdup("*=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '/' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_SLASH_EQ;
            t.text = xstrdup("/=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '%' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_PERCENT_EQ;
            t.text = xstrdup("%=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '&' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_AMP_EQ;
            t.text = xstrdup("&=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '|' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_BITOR_EQ;
            t.text = xstrdup("|=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '^' && source[pos + 1] == '=') {
            Token t;
            t.kind = TK_XOR_EQ;
            t.text = xstrdup("^=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '<' && source[pos + 1] == '<' && source[pos + 2] == '=') {
            Token t;
            t.kind = TK_SHL_EQ;
            t.text = xstrdup("<<=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 3; col += 3;
            goto lex_loop_head;
        }
        if (c == '>' && source[pos + 1] == '>' && source[pos + 2] == '=') {
            Token t;
            t.kind = TK_SHR_EQ;
            t.text = xstrdup(">>=");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 3; col += 3;
            goto lex_loop_head;
        }
        if (c == '<' && source[pos + 1] == '<') {
            Token t;
            t.kind = TK_SHL;
            t.text = xstrdup("<<");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
        }
        if (c == '>' && source[pos + 1] == '>') {
            Token t;
            t.kind = TK_SHR;
            t.text = xstrdup(">>");
            t.loc.file = filename; t.loc.line = line; t.loc.col = col;
            token_array_push(out, t);
            pos += 2; col += 2;
            goto lex_loop_head;
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
            goto lex_loop_head;
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
            goto lex_loop_head;
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
            case '.':
                /* `...` is the variadic ellipsis. The float-literal path
                 * (`.5`) only consumes a `.` followed by a digit, so a bare
                 * `...` always reaches this general case. */
                if (source[pos + 1] == '.' && source[pos + 2] == '.') {
                    t.kind = TK_ELLIPSIS;
                    t.text = xstrdup("...");
                    t.loc.file = filename;
                    t.loc.line = line;
                    t.loc.col = col;
                    token_array_push(out, t);
                    pos += 3; col += 3;
                    goto lex_loop_head;
                }
                t.kind = TK_DOT;
                break;
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
            goto lex_loop_head;
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
