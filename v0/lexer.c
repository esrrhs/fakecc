package main;

static int __fakecc_ctzll(unsigned long _v){int c;for(c=0;!(_v&1);c++)_v>>=1;return c;}
static void __fakecc_va_copy(void *dst, void *src){
    char *d = (char*)dst; char *s = (char*)src;
    for(int i = 0; i < 24; i++) d[i] = s[i];
}

typedef long ptrdiff_t;
typedef unsigned long size_t;
typedef long ssize_t;
typedef long intptr_t;
typedef unsigned long uintptr_t;
struct SourceLoc {
    const char *file;
    int line;
    int col;
};typedef struct SourceLoc SourceLoc;
struct Buffer {
    char *data;
    size_t len;
    size_t cap;
};typedef struct Buffer Buffer;
void buffer_init(Buffer *b);
void buffer_free(Buffer *b);
void buffer_append(Buffer *b, const char *s, size_t n);
void buffer_appendf(Buffer *b, const char *fmt, ...);
char *xstrdup(const char *s);
void *xmalloc(size_t n);
void *xrealloc(void *p, size_t n);
void die_at(const char *file, int line, int col, const char *fmt, ...);
enum TokenKind {
    TK_KW_PACKAGE,
    TK_KW_IMPORT,
    TK_KW_VOID,
    TK_KW_INT,
    TK_KW_FLOAT,
    TK_KW_DOUBLE,
    TK_KW_CHAR,
    TK_KW_SHORT,
    TK_KW_LONG,
    TK_KW_SIGNED,
    TK_KW_UNSIGNED,
    TK_KW_SIZEOF,
    TK_KW_RETURN,
    TK_KW_IF,
    TK_KW_ELSE,
    TK_KW_WHILE,
    TK_KW_FOR,
    TK_KW_GOTO,
    TK_KW_SWITCH,
    TK_KW_CASE,
    TK_KW_DEFAULT,
    TK_KW_BREAK,
    TK_KW_CONTINUE,
    TK_KW_CONST,
    TK_KW_UNION,
    TK_KW_DO,
    TK_KW_ENUM,
    TK_KW_STRUCT,
    TK_KW_TYPEDEF,
    TK_KW_STATIC,
    TK_KW_EXTERN,
    TK_KW_BOOL,
    TK_KW_VOLATILE,
    TK_KW_RESTRICT,
    TK_KW_INLINE,
    TK_KW_ALIGNOF,
    TK_KW_LONG_DOUBLE,
    TK_IDENT,
    TK_INT_LITERAL,
    TK_FLOAT_LITERAL,
    TK_CHAR_LITERAL,
    TK_STRING_LITERAL,
    TK_LPAREN,
    TK_RPAREN,
    TK_LBRACE,
    TK_RBRACE,
    TK_LBRACKET,
    TK_RBRACKET,
    TK_SEMICOLON,
    TK_COMMA,
    TK_PLUS,
    TK_MINUS,
    TK_STAR,
    TK_SLASH,
    TK_PERCENT,
    TK_AMP,
    TK_ANDAND,
    TK_OROR,
    TK_BITOR,
    TK_XOR,
    TK_TILDE,
    TK_NOT,
    TK_SHL,
    TK_SHR,
    TK_SHL_EQ,
    TK_SHR_EQ,
    TK_PLUS_EQ,
    TK_MINUS_EQ,
    TK_STAR_EQ,
    TK_SLASH_EQ,
    TK_PERCENT_EQ,
    TK_AMP_EQ,
    TK_BITOR_EQ,
    TK_XOR_EQ,
    TK_INC,
    TK_DEC,
    TK_QUESTION,
    TK_COLON,
    TK_DOT,
    TK_ELLIPSIS,
    TK_ARROW,
    TK_ASSIGN,
    TK_EQ,
    TK_NE,
    TK_LT,
    TK_LE,
    TK_GT,
    TK_GE,
    TK_EOF,
};typedef enum TokenKind TokenKind;
struct Token {
    TokenKind kind;
    char *text;
    SourceLoc loc;
};typedef struct Token Token;
struct TokenArray {
    Token *data;
    size_t len;
    size_t cap;
};typedef struct TokenArray TokenArray;
void token_array_init(TokenArray *a);
void token_array_free(TokenArray *a);
void token_array_push(TokenArray *a, Token t);
void lex(const char *source, const char *filename, TokenArray *out);
extern int isdigit(int c);
extern int isalpha(int c);
extern int isalnum(int c);
extern int isxdigit(int c);
extern int isspace(int c);
typedef struct FILE FILE;
extern FILE *stderr;
extern FILE *stdin;
extern FILE *stdout;
extern int fprintf(FILE *f, const char *fmt, ...);
extern int vfprintf(FILE *f, const char *fmt, va_list ap);
extern int printf(const char *fmt, ...);
extern int sprintf(char *buf, const char *fmt, ...);
extern int snprintf(char *buf, size_t n, const char *fmt, ...);
extern int fputs(const char *s, FILE *f);
extern int fputc(int c, FILE *f);
extern int fflush(FILE *f);
extern int puts(const char *s);
extern int putchar(int c);
extern FILE *fopen(const char *p, const char *m);
extern int fclose(FILE *f);
extern size_t fwrite(const void *p, size_t n, size_t m, FILE *f);
extern size_t fread(void *p, size_t n, size_t m, FILE *f);
extern void perror(const char *s);
extern int fileno(FILE *f);
extern int fseek(FILE *f, long off, int whence);
extern long ftell(FILE *f);
typedef long fpos_t;
extern void *malloc(size_t n);
extern void *realloc(void *p, size_t n);
extern void *calloc(size_t n, size_t m);
extern void free(void *p);
extern void exit(int code);
extern void abort(void);
extern int atoi(const char *s);
extern long atol(const char *s);
extern long strtol(const char *s, char **end, int base);
extern unsigned long strtoul(const char *s, char **end, int base);
extern unsigned long long strtoull(const char *s, char **end, int base);
extern double strtod(const char *s, char **end);
extern float strtof(const char *s, char **end);
extern long double strtold(const char *nptr, char **endptr);
extern void qsort(void *base, size_t n, size_t sz, int (*cmp)(const void*, const void*));
extern char *getenv(const char *name);
extern void *memcpy(void *dst, const void *src, size_t n);
extern void *memmove(void *dst, const void *src, size_t n);
extern void *memset(void *dst, int c, size_t n);
extern int memcmp(const void *a, const void *b, size_t n);
extern size_t strlen(const char *s);
extern char *strdup(const char *s);
extern int strcmp(const char *a, const char *b);
extern int strncmp(const char *a, const char *b, size_t n);
extern char *strchr(const char *s, int c);
extern char *strrchr(const char *s, int c);
extern char *strstr(const char *a, const char *b);
extern char *strcpy(char *dst, const char *src);
extern char *strncpy(char *dst, const char *src, size_t n);
extern char *strerror(int n);
void token_array_init(TokenArray *a) {
    a->data = ((void*)0);
    a->len = 0;
    a->cap = 0;
}
void token_array_free(TokenArray *a) {
    for (size_t i = 0; i < a->len; i++) {
        free(a->data[i].text);
    }
    free(a->data);
    a->data = ((void*)0);
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
        break;
    case 7:
        if (memcmp(s, "package", 7) == 0) return TK_KW_PACKAGE;
        if (memcmp(s, "default", 7) == 0) return TK_KW_DEFAULT;
        if (memcmp(s, "typedef", 7) == 0) return TK_KW_TYPEDEF;
        break;
    case 8:
        if (memcmp(s, "unsigned", 8) == 0) return TK_KW_UNSIGNED;
        if (memcmp(s, "continue", 8) == 0) return TK_KW_CONTINUE;
        if (memcmp(s, "volatile", 8) == 0) return TK_KW_VOLATILE;
        if (memcmp(s, "restrict", 8) == 0) return TK_KW_RESTRICT;
        if (memcmp(s, "_Alignof", 8) == 0) return TK_KW_ALIGNOF;
        break;
    default:
        break;
    }
    return TK_IDENT;
}
void lex(const char *source, const char *filename, TokenArray *out) {
    size_t pos = 0;
    int line = 1;
    int col = 1;
    int line_start = 1;
    char c;
lex_loop_head:
    while (source[pos] != '\0') {
        c = source[pos];
        if (c == '\n') {
            pos++;
            line++;
            col = 1;
            line_start = 1;
            goto lex_loop_head;
        }
        if (c == ' ' || c == '\t' || c == '\r') {
            pos++;
            col++;
            goto lex_loop_head;
        }
        if (c == '/' && source[pos + 1] == '/') {
            pos += 2;
            col += 2;
            while (source[pos] != '\0' && source[pos] != '\n') {
                pos++;
                col++;
            }
            goto lex_loop_head;
        }
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
        if (c == '#' && line_start) {
            int start_line = line;
            int start_col = col;
            die_at(filename, start_line, start_col,
                   "preprocessor directives are not supported in FakeCC");
        }
        line_start = 0;
        if (c == '\'') {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            pos++;
            col++;
            if (source[pos] == '\0' || source[pos] == '\n') {
                die_at(filename, start_line, start_col,
                       "unterminated character literal");
            }
            if (source[pos] == '\\') {
                pos++; col++;
                if (source[pos] == '\0' || source[pos] == '\n') {
                    die_at(filename, start_line, start_col,
                           "unterminated character literal");
                }
                if (source[pos] == 'x' || source[pos] == 'X') {
                    pos++; col++;
                    if (!isxdigit((unsigned char)source[pos]))
                        die_at(filename, start_line, start_col,
                               "hex escape \\x with no digits");
                    while (isxdigit((unsigned char)source[pos])) {
                        pos++; col++;
                    }
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
            if (source[pos] != '\'') {
                die_at(filename, start_line, start_col,
                       "missing closing quote in character literal");
            }
            pos++; col++;
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
        if (c == '"') {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            pos++;
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
        if (isdigit((unsigned char)c)) {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            if (c == '0' && (source[pos + 1] == 'x' || source[pos + 1] == 'X')) {
                pos += 2;
                col += 2;
                if (!isxdigit((unsigned char)source[pos]))
                    die_at(filename, start_line, start_col,
                           "hex literal has no digits");
                while (isxdigit((unsigned char)source[pos])) { pos++; col++; }
            } else {
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            int is_float = 0;
            if (source[pos] == '.') {
                is_float = 1;
                pos++; col++;
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            if (source[pos] == 'e' || source[pos] == 'E') {
                is_float = 1;
                pos++; col++;
                if (source[pos] == '+' || source[pos] == '-') { pos++; col++; }
                if (!isdigit((unsigned char)source[pos])) {
                    die_at(filename, line, col,
                           "exponent has no digits");
                }
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            if (!is_float) {
                int saw_u = 0, saw_l = 0;
                for (;;) {
                    if ((source[pos] == 'u' || source[pos] == 'U') && !saw_u) {
                        saw_u = 1; pos++; col++;
                    } else if ((source[pos] == 'l' || source[pos] == 'L')
                               && saw_l < 2) {
                        saw_l++; pos++; col++;
                    } else break;
                }
            } else {
                if (source[pos] == 'f' || source[pos] == 'F') {
                    pos++; col++;
                } else if (source[pos] == 'l' || source[pos] == 'L') {
                    pos++; col++;
                }
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
        if (c == '.' && isdigit((unsigned char)source[pos + 1])) {
            int start_line = line;
            int start_col = col;
            size_t start = pos;
            pos++; col++;
            while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            if (source[pos] == 'e' || source[pos] == 'E') {
                pos++; col++;
                if (source[pos] == '+' || source[pos] == '-') { pos++; col++; }
                if (!isdigit((unsigned char)source[pos])) {
                    die_at(filename, line, col, "exponent has no digits");
                }
                while (isdigit((unsigned char)source[pos])) { pos++; col++; }
            }
            if (source[pos] == 'f' || source[pos] == 'F') { pos++; col++; }
            else if (source[pos] == 'l' || source[pos] == 'L') { pos++; col++; }
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
            goto lex_loop_head;
        }
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
            default: t.kind = TK_EOF; break;
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
        die_at(filename, line, col, "unexpected character '%c'", c);
    }
    Token eof;
    eof.kind = TK_EOF;
    eof.text = malloc(1);
    eof.text[0] = '\0';
    eof.loc.file = filename;
    eof.loc.line = line;
    eof.loc.col = col;
    token_array_push(out, eof);
}
