/*
 * tiny-regex-c — Mini regex engine inspired by Rob Pike's implementation
 * FakeCC port of https://github.com/kokke/tiny-regex-c
 *
 * This is free and unencumbered software released into the public domain.
 *
 * Anyone is free to copy, modify, publish, use, compile, sell, or
 * distribute this software, either in source code form or as a compiled
 * binary, for any purpose, commercial or non-commercial, and by any
 * means.
 *
 * In jurisdictions that recognize copyright laws, the author or authors
 * of this software dedicate any and all copyright interest in the
 * software to the public domain. We make this dedication for the benefit
 * of the public at large and to the detriment of our heirs and
 * successors. We intend this dedication to be an overt act of
 * relinquishment in perpetuity of all present and future rights to this
 * software under copyright law.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 *
 * For more information, please refer to <http://unlicense.org/>
 *
 * Supports:
 *   '.'        Dot, matches any character
 *   '^'        Start anchor
 *   '$'        End anchor
 *   '*'        Zero or more (greedy)
 *   '+'        One or more (greedy)
 *   '?'        Zero or one
 *   '[abc]'    Character class
 *   '[^abc]'   Inverted class (upstream notes this as currently broken)
 *   '[a-zA-Z]' Character ranges
 *   '\s' '\S' '\w' '\W' '\d' '\D'
 *
 * FakeCC adaptations:
 *   - package tinyregex; import runtime (no preprocessor / headers)
 *   - MAX_REGEXP_OBJECTS / MAX_CHAR_CLASS_LEN frozen as enum
 *   - RE_DOT_MATCHES_NEWLINE frozen on ('.' matches '\n' / '\r')
 */

package tinyregex;

import runtime;

enum {
    MAX_REGEXP_OBJECTS = 30,
    MAX_CHAR_CLASS_LEN = 40
};

enum {
    UNUSED,
    DOT,
    BEGIN,
    END,
    QUESTIONMARK,
    STAR,
    PLUS,
    CHAR,
    CHAR_CLASS,
    INV_CHAR_CLASS,
    DIGIT,
    NOT_DIGIT,
    ALPHA,
    NOT_ALPHA,
    WHITESPACE,
    NOT_WHITESPACE
};

typedef struct regex_t {
    unsigned char type;
    union {
        unsigned char ch;
        unsigned char *ccl;
    } u;
} regex_t;

typedef struct regex_t *re_t;

re_t re_compile(const char *pattern);
int re_matchp(re_t pattern, const char *text, int *matchlength);
int re_match(const char *pattern, const char *text, int *matchlength);
void re_print(regex_t *pattern);

static int matchpattern(regex_t *pattern, const char *text, int *matchlength);
static int matchcharclass(char c, const char *str);
static int matchstar(regex_t p, regex_t *pattern, const char *text, int *matchlength);
static int matchplus(regex_t p, regex_t *pattern, const char *text, int *matchlength);
static int matchquestion(regex_t p, regex_t *pattern, const char *text, int *matchlength);
static int matchone(regex_t p, char c);
static int matchdigit(char c);
static int matchalpha(char c);
static int matchwhitespace(char c);
static int matchalphanum(char c);
static int matchmetachar(char c, const char *str);
static int matchrange(char c, const char *str);
static int matchdot(char c);
static int ismetachar(char c);

int re_match(const char *pattern, const char *text, int *matchlength) {
    return re_matchp(re_compile(pattern), text, matchlength);
}

int re_matchp(re_t pattern, const char *text, int *matchlength) {
    int idx;

    *matchlength = 0;
    if (pattern != 0) {
        if (pattern[0].type == BEGIN) {
            return matchpattern(&pattern[1], text, matchlength) ? 0 : -1;
        } else {
            idx = -1;
            do {
                idx += 1;
                if (matchpattern(pattern, text, matchlength)) {
                    if (text[0] == '\0')
                        return -1;
                    return idx;
                }
            } while (*text++ != '\0');
        }
    }
    return -1;
}

re_t re_compile(const char *pattern) {
    static regex_t re_compiled[30];
    static unsigned char ccl_buf[40];
    int ccl_bufidx;
    char c;
    int i;
    int j;
    int buf_begin;

    ccl_bufidx = 1;
    i = 0;
    j = 0;

    while (pattern[i] != '\0' && (j + 1 < MAX_REGEXP_OBJECTS)) {
        c = pattern[i];

        switch (c) {
        case '^':
            re_compiled[j].type = BEGIN;
            break;
        case '$':
            re_compiled[j].type = END;
            break;
        case '.':
            re_compiled[j].type = DOT;
            break;
        case '*':
            re_compiled[j].type = STAR;
            break;
        case '+':
            re_compiled[j].type = PLUS;
            break;
        case '?':
            re_compiled[j].type = QUESTIONMARK;
            break;
        case '\\':
            if (pattern[i + 1] != '\0') {
                i += 1;
                switch (pattern[i]) {
                case 'd':
                    re_compiled[j].type = DIGIT;
                    break;
                case 'D':
                    re_compiled[j].type = NOT_DIGIT;
                    break;
                case 'w':
                    re_compiled[j].type = ALPHA;
                    break;
                case 'W':
                    re_compiled[j].type = NOT_ALPHA;
                    break;
                case 's':
                    re_compiled[j].type = WHITESPACE;
                    break;
                case 'S':
                    re_compiled[j].type = NOT_WHITESPACE;
                    break;
                default:
                    re_compiled[j].type = CHAR;
                    re_compiled[j].u.ch = pattern[i];
                    break;
                }
            }
            break;
        case '[':
            buf_begin = ccl_bufidx;
            if (pattern[i + 1] == '^') {
                re_compiled[j].type = INV_CHAR_CLASS;
                i += 1;
                if (pattern[i + 1] == 0) {
                    return 0;
                }
            } else {
                re_compiled[j].type = CHAR_CLASS;
            }
            while ((pattern[++i] != ']') && (pattern[i] != '\0')) {
                if (pattern[i] == '\\') {
                    if (ccl_bufidx >= MAX_CHAR_CLASS_LEN - 1) {
                        return 0;
                    }
                    if (pattern[i + 1] == 0) {
                        return 0;
                    }
                    ccl_buf[ccl_bufidx++] = pattern[i++];
                } else if (ccl_bufidx >= MAX_CHAR_CLASS_LEN) {
                    return 0;
                }
                ccl_buf[ccl_bufidx++] = pattern[i];
            }
            if (ccl_bufidx >= MAX_CHAR_CLASS_LEN) {
                return 0;
            }
            ccl_buf[ccl_bufidx++] = 0;
            re_compiled[j].u.ccl = &ccl_buf[buf_begin];
            break;
        default:
            re_compiled[j].type = CHAR;
            re_compiled[j].u.ch = c;
            break;
        }

        if (pattern[i] == 0) {
            return 0;
        }

        i += 1;
        j += 1;
    }
    re_compiled[j].type = UNUSED;
    return re_compiled;
}

void re_print(regex_t *pattern) {
    const char *types[16];
    int i;
    int j;
    char c;

    types[0] = "UNUSED";
    types[1] = "DOT";
    types[2] = "BEGIN";
    types[3] = "END";
    types[4] = "QUESTIONMARK";
    types[5] = "STAR";
    types[6] = "PLUS";
    types[7] = "CHAR";
    types[8] = "CHAR_CLASS";
    types[9] = "INV_CHAR_CLASS";
    types[10] = "DIGIT";
    types[11] = "NOT_DIGIT";
    types[12] = "ALPHA";
    types[13] = "NOT_ALPHA";
    types[14] = "WHITESPACE";
    types[15] = "NOT_WHITESPACE";

    for (i = 0; i < MAX_REGEXP_OBJECTS; ++i) {
        if (pattern[i].type == UNUSED) {
            break;
        }
        runtime.printf("type: %s", types[pattern[i].type]);
        if (pattern[i].type == CHAR_CLASS || pattern[i].type == INV_CHAR_CLASS) {
            runtime.printf(" [");
            for (j = 0; j < MAX_CHAR_CLASS_LEN; ++j) {
                c = pattern[i].u.ccl[j];
                if ((c == '\0') || (c == ']')) {
                    break;
                }
                runtime.printf("%c", c);
            }
            runtime.printf("]");
        } else if (pattern[i].type == CHAR) {
            runtime.printf(" '%c'", pattern[i].u.ch);
        }
        runtime.printf("\n");
    }
}

static int matchdigit(char c) {
    return runtime.isdigit(c);
}

static int matchalpha(char c) {
    return runtime.isalpha(c);
}

static int matchwhitespace(char c) {
    return runtime.isspace(c);
}

static int matchalphanum(char c) {
    return (c == '_') || matchalpha(c) || matchdigit(c);
}

static int matchrange(char c, const char *str) {
    return (c != '-')
        && (str[0] != '\0')
        && (str[0] != '-')
        && (str[1] == '-')
        && (str[2] != '\0')
        && (c >= str[0])
        && (c <= str[2]);
}

static int matchdot(char c) {
    /* RE_DOT_MATCHES_NEWLINE frozen on. */
    return 1;
}

static int ismetachar(char c) {
    return (c == 's') || (c == 'S') || (c == 'w') || (c == 'W') || (c == 'd') || (c == 'D');
}

static int matchmetachar(char c, const char *str) {
    switch (str[0]) {
    case 'd':
        return matchdigit(c);
    case 'D':
        return !matchdigit(c);
    case 'w':
        return matchalphanum(c);
    case 'W':
        return !matchalphanum(c);
    case 's':
        return matchwhitespace(c);
    case 'S':
        return !matchwhitespace(c);
    default:
        return (c == str[0]);
    }
}

static int matchcharclass(char c, const char *str) {
    do {
        if (matchrange(c, str)) {
            return 1;
        } else if (str[0] == '\\') {
            str += 1;
            if (matchmetachar(c, str)) {
                return 1;
            } else if ((c == str[0]) && !ismetachar(c)) {
                return 1;
            }
        } else if (c == str[0]) {
            if (c == '-') {
                return (str[-1] == '\0') || (str[1] == '\0');
            } else {
                return 1;
            }
        }
    } while (*str++ != '\0');
    return 0;
}

static int matchone(regex_t p, char c) {
    switch (p.type) {
    case DOT:
        return matchdot(c);
    case CHAR_CLASS:
        return matchcharclass(c, (const char *)p.u.ccl);
    case INV_CHAR_CLASS:
        return !matchcharclass(c, (const char *)p.u.ccl);
    case DIGIT:
        return matchdigit(c);
    case NOT_DIGIT:
        return !matchdigit(c);
    case ALPHA:
        return matchalphanum(c);
    case NOT_ALPHA:
        return !matchalphanum(c);
    case WHITESPACE:
        return matchwhitespace(c);
    case NOT_WHITESPACE:
        return !matchwhitespace(c);
    default:
        return (p.u.ch == c);
    }
}

static int matchstar(regex_t p, regex_t *pattern, const char *text, int *matchlength) {
    int prelen;
    const char *prepoint;

    prelen = *matchlength;
    prepoint = text;
    while ((text[0] != '\0') && matchone(p, *text)) {
        text++;
        (*matchlength)++;
    }
    while (text >= prepoint) {
        if (matchpattern(pattern, text--, matchlength))
            return 1;
        (*matchlength)--;
    }
    *matchlength = prelen;
    return 0;
}

static int matchplus(regex_t p, regex_t *pattern, const char *text, int *matchlength) {
    const char *prepoint;

    prepoint = text;
    while ((text[0] != '\0') && matchone(p, *text)) {
        text++;
        (*matchlength)++;
    }
    while (text > prepoint) {
        if (matchpattern(pattern, text--, matchlength))
            return 1;
        (*matchlength)--;
    }
    return 0;
}

static int matchquestion(regex_t p, regex_t *pattern, const char *text, int *matchlength) {
    if (p.type == UNUSED)
        return 1;
    if (matchpattern(pattern, text, matchlength))
        return 1;
    if (*text && matchone(p, *text++)) {
        if (matchpattern(pattern, text, matchlength)) {
            (*matchlength)++;
            return 1;
        }
    }
    return 0;
}

static int matchpattern(regex_t *pattern, const char *text, int *matchlength) {
    int pre;

    pre = *matchlength;
    do {
        if ((pattern[0].type == UNUSED) || (pattern[1].type == QUESTIONMARK)) {
            return matchquestion(pattern[0], &pattern[2], text, matchlength);
        } else if (pattern[1].type == STAR) {
            return matchstar(pattern[0], &pattern[2], text, matchlength);
        } else if (pattern[1].type == PLUS) {
            return matchplus(pattern[0], &pattern[2], text, matchlength);
        } else if ((pattern[0].type == END) && pattern[1].type == UNUSED) {
            return (text[0] == '\0');
        }
        (*matchlength)++;
    } while ((text[0] != '\0') && matchone(*pattern++, *text++));

    *matchlength = pre;
    return 0;
}
