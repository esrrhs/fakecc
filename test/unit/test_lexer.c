#include "fakecc/lexer.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

/* ---- helper ---- */
static TokenArray lex_str(const char *src) {
    TokenArray arr;
    token_array_init(&arr);
    lex(src, "test.c", &arr);
    return arr;
}

/* ---- tests ---- */

static void test_keyword_package(void) {
    TokenArray a = lex_str("package");
    T_ASSERT_EQ_INT((int)a.len, 2); /* TK_KW_PACKAGE + TK_EOF */
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_PACKAGE);
    T_ASSERT_STR_EQ(a.data[0].text, "package");
    token_array_free(&a);
}

static void test_keyword_int(void) {
    TokenArray a = lex_str("int");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_INT);
    T_ASSERT_STR_EQ(a.data[0].text, "int");
    token_array_free(&a);
}

static void test_keyword_return(void) {
    TokenArray a = lex_str("return");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_RETURN);
    T_ASSERT_STR_EQ(a.data[0].text, "return");
    token_array_free(&a);
}

static void test_int_literal(void) {
    TokenArray a = lex_str("42");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_INT_LITERAL);
    T_ASSERT_STR_EQ(a.data[0].text, "42");
    token_array_free(&a);
}

static void test_full_program(void) {
    TokenArray a = lex_str("package main; int main() { return 42; }");
    /* package main ; int main ( ) { return 42 ; } EOF = 13 tokens */
    T_ASSERT_EQ_INT((int)a.len, 13);
    T_ASSERT_EQ_INT((int)a.data[0].kind,  (int)TK_KW_PACKAGE);
    T_ASSERT_EQ_INT((int)a.data[1].kind,  (int)TK_IDENT);
    T_ASSERT_STR_EQ(a.data[1].text, "main");
    T_ASSERT_EQ_INT((int)a.data[2].kind,  (int)TK_SEMICOLON);
    T_ASSERT_EQ_INT((int)a.data[3].kind,  (int)TK_KW_INT);
    T_ASSERT_EQ_INT((int)a.data[4].kind,  (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind,  (int)TK_LPAREN);
    T_ASSERT_EQ_INT((int)a.data[6].kind,  (int)TK_RPAREN);
    T_ASSERT_EQ_INT((int)a.data[7].kind,  (int)TK_LBRACE);
    T_ASSERT_EQ_INT((int)a.data[8].kind,  (int)TK_KW_RETURN);
    T_ASSERT_EQ_INT((int)a.data[9].kind,  (int)TK_INT_LITERAL);
    T_ASSERT_STR_EQ(a.data[9].text, "42");
    T_ASSERT_EQ_INT((int)a.data[10].kind, (int)TK_SEMICOLON);
    T_ASSERT_EQ_INT((int)a.data[11].kind, (int)TK_RBRACE);
    T_ASSERT_EQ_INT((int)a.data[12].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_empty_input(void) {
    TokenArray a = lex_str("");
    T_ASSERT_EQ_INT((int)a.len, 1);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_line_comment(void) {
    TokenArray a = lex_str("// hi\npackage");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_PACKAGE);
    token_array_free(&a);
}

static void test_block_comment(void) {
    TokenArray a = lex_str("/* comment */package");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_PACKAGE);
    token_array_free(&a);
}

static void test_position_tracking(void) {
    TokenArray a = lex_str("package\nint x");
    /* "package" at line 1, col 1 */
    T_ASSERT_EQ_INT(a.data[0].loc.line, 1);
    T_ASSERT_EQ_INT(a.data[0].loc.col, 1);
    /* "int" at line 2, col 1 */
    T_ASSERT_EQ_INT(a.data[1].loc.line, 2);
    T_ASSERT_EQ_INT(a.data[1].loc.col, 1);
    /* "x" at line 2, col 5 */
    T_ASSERT_EQ_INT(a.data[2].loc.line, 2);
    T_ASSERT_EQ_INT(a.data[2].loc.col, 5);
    token_array_free(&a);
}

static void test_string_literal(void) {
    TokenArray a = lex_str("\"hello\"");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_STRING_LITERAL);
    T_ASSERT_STR_EQ(a.data[0].text, "\"hello\"");
    token_array_free(&a);
}

static void test_char_literal_simple(void) {
    TokenArray a = lex_str("'A'");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_CHAR_LITERAL);
    T_ASSERT_STR_EQ(a.data[0].text, "'A'");
    token_array_free(&a);
}

static void test_char_literal_escape_n(void) {
    TokenArray a = lex_str("'\\n'");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_CHAR_LITERAL);
    T_ASSERT_STR_EQ(a.data[0].text, "'\\n'");
    token_array_free(&a);
}

static void test_char_literal_escape_backslash(void) {
    TokenArray a = lex_str("'\\\\'");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_CHAR_LITERAL);
    T_ASSERT_STR_EQ(a.data[0].text, "'\\\\'");
    token_array_free(&a);
}

static void test_char_literal_in_expr(void) {
    /* 'A'+1 → CHAR_LITERAL PLUS INT_LITERAL EOF */
    TokenArray a = lex_str("'A'+1");
    T_ASSERT_EQ_INT((int)a.len, 4);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_CHAR_LITERAL);
    T_ASSERT_STR_EQ(a.data[0].text, "'A'");
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_PLUS);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_INT_LITERAL);
    T_ASSERT_STR_EQ(a.data[2].text, "1");
    token_array_free(&a);
}

static void test_keyword_import(void) {
    TokenArray a = lex_str("import");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_IMPORT);
    T_ASSERT_STR_EQ(a.data[0].text, "import");
    token_array_free(&a);
}

static void test_unknown_char_dies(void) {
    /* unknown character '@' should cause die_at and exit */
    int pid = fork();
    if (pid == 0) {
        TokenArray a;
        token_array_init(&a);
        lex("@", "test.c", &a);
        token_array_free(&a);
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

static void test_preprocessor_rejected(void) {
    /* #include <stdio.h> should be rejected with specific error */
    int pid = fork();
    if (pid == 0) {
        TokenArray a;
        token_array_init(&a);
        lex("#include <stdio.h>", "test.c", &a);
        token_array_free(&a);
        _exit(0);
    }
    int status;
    waitpid(pid, &status, 0);
    T_ASSERT(WIFEXITED(status) && WEXITSTATUS(status) != 0);
}

/* ---- Slice 2: arithmetic operators ---- */

static void test_arith_op_tokens(void) {
    /* "1+2*3-4/5%6" → INT PLUS INT STAR INT MINUS INT SLASH INT PERCENT INT EOF */
    TokenArray a = lex_str("1+2*3-4/5%6");
    T_ASSERT_EQ_INT((int)a.len, 12);
    T_ASSERT_EQ_INT((int)a.data[0].kind,  (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[1].kind,  (int)TK_PLUS);
    T_ASSERT_EQ_INT((int)a.data[2].kind,  (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[3].kind,  (int)TK_STAR);
    T_ASSERT_EQ_INT((int)a.data[4].kind,  (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[5].kind,  (int)TK_MINUS);
    T_ASSERT_EQ_INT((int)a.data[6].kind,  (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[7].kind,  (int)TK_SLASH);
    T_ASSERT_EQ_INT((int)a.data[8].kind,  (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[9].kind,  (int)TK_PERCENT);
    T_ASSERT_EQ_INT((int)a.data[10].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[11].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_paren_expr_tokens(void) {
    /* "(1+2)" → LPAREN INT PLUS INT RPAREN EOF */
    TokenArray a = lex_str("(1+2)");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_LPAREN);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_PLUS);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_RPAREN);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_comment_with_arith(void) {
    /* "1 // comment\n+2" → comment eaten, tokens: INT PLUS INT EOF */
    TokenArray a = lex_str("1 // comment\n+2");
    T_ASSERT_EQ_INT((int)a.len, 4);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_PLUS);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_slash_not_comment(void) {
    /* "1/2" → INT SLASH INT (not a comment) */
    TokenArray a = lex_str("1/2");
    T_ASSERT_EQ_INT((int)a.len, 4);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_SLASH);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_EOF);
    token_array_free(&a);
}

/* ---- Slice 3: assignment operator ---- */

static void test_assign_token(void) {
    /* "x = 5" → IDENT ASSIGN INT EOF */
    TokenArray a = lex_str("x = 5");
    T_ASSERT_EQ_INT((int)a.len, 4);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_STR_EQ(a.data[0].text, "x");
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_ASSIGN);
    T_ASSERT_STR_EQ(a.data[1].text, "=");
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_INT_LITERAL);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_double_assign_is_two_tokens(void) {
    /* "a==b" → IDENT EQ IDENT EOF ("==" is a single token now) */
    TokenArray a = lex_str("a==b");
    T_ASSERT_EQ_INT((int)a.len, 4);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_EQ);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_EOF);
    token_array_free(&a);
}

/* ---- Comparison operators & control-flow keywords ---- */

static void test_compare_op_tokens(void) {
    /* "a<b<=c==d!=e>=f>g" → IDENT LT IDENT LE IDENT EQ IDENT NE IDENT GE IDENT GT IDENT EOF */
    TokenArray a = lex_str("a<b<=c==d!=e>=f>g");
    T_ASSERT_EQ_INT((int)a.data[0].kind,  (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind,  (int)TK_LT);
    T_ASSERT_EQ_INT((int)a.data[2].kind,  (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind,  (int)TK_LE);
    T_ASSERT_EQ_INT((int)a.data[4].kind,  (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind,  (int)TK_EQ);
    T_ASSERT_EQ_INT((int)a.data[6].kind,  (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[7].kind,  (int)TK_NE);
    T_ASSERT_EQ_INT((int)a.data[8].kind,  (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[9].kind,  (int)TK_GE);
    T_ASSERT_EQ_INT((int)a.data[10].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[11].kind, (int)TK_GT);
    T_ASSERT_EQ_INT((int)a.data[12].kind, (int)TK_IDENT);
    token_array_free(&a);
}

static void test_control_flow_keywords(void) {
    TokenArray a = lex_str("if else while");
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_IF);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_KW_ELSE);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_KW_WHILE);
    token_array_free(&a);
}

static void test_not_token(void) {
    /* "!x" → NOT IDENT EOF */
    TokenArray a = lex_str("!x");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_NOT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_not_not_confused_with_ne(void) {
    /* "!==": the lexer matches "!=" first (TK_NE), leaving "=" (TK_ASSIGN).
     * Confirms != is a single token and ! does NOT combine with == . */
    TokenArray a = lex_str("!==");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_NE);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_ASSIGN);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_logical_op_tokens(void) {
    TokenArray a = lex_str("a && b || c");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_ANDAND);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_OROR);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_andand_not_confused_with_amp(void) {
    /* "&&&" should lex as TK_ANDAND followed by TK_AMP (address-of). */
    TokenArray a = lex_str("&&&");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_ANDAND);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_AMP);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_bitwise_op_tokens(void) {
    /* "a|b^c&d" → IDENT BITOR IDENT XOR IDENT AMP IDENT EOF (= 8) */
    TokenArray a = lex_str("a|b^c&d");
    T_ASSERT_EQ_INT((int)a.len, 8);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_BITOR);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_XOR);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_AMP);
    T_ASSERT_EQ_INT((int)a.data[6].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[7].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_tilde_not_confused(void) {
    /* "~x" → TILDE IDENT EOF */
    TokenArray a = lex_str("~x");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_TILDE);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_shl_shr_tokens(void) {
    /* "a<<b>>c" → IDENT SHL IDENT SHR IDENT EOF */
    TokenArray a = lex_str("a<<b>>c");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_SHL);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_SHR);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_shl_not_confused_with_lt(void) {
    /* "<<<<" should lex as SHL + SHL + EOF */
    TokenArray a = lex_str("<<<<");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_SHL);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_SHL);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_bitor_not_confused_with_oror(void) {
    /* "|||" → the lexer greedily matches "||" first, then "|":
     * OROR + BITOR + EOF. This confirms single | is a distinct token. */
    TokenArray a = lex_str("|||");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_OROR);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_BITOR);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_compound_assign_tokens(void) {
    /* "a+=b" → IDENT PLUS_EQ IDENT EOF (= 4) */
    TokenArray a = lex_str("a+=b");
    T_ASSERT_EQ_INT((int)a.len, 4);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_PLUS_EQ);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_shift_compound_tokens(void) {
    /* "a<<=b>>=c" → IDENT SHL_EQ IDENT SHR_EQ IDENT EOF (= 6) */
    TokenArray a = lex_str("a<<=b>>=c");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_SHL_EQ);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_SHR_EQ);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_compound_not_confused(void) {
    /* "++=" should lex as INC + ASSIGN, not a single token. */
    TokenArray a = lex_str("++=");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_INC);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_ASSIGN);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_inc_dec_tokens(void) {
    /* "a++ + ++b" → IDENT INC PLUS INC IDENT EOF */
    TokenArray a = lex_str("a++ + ++b");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_INC);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_PLUS);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_INC);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_dec_not_confused_with_arrow(void) {
    /* "---" should lex as DEC + MINUS + EOF (-- then -), NOT arrow. */
    TokenArray a = lex_str("---");
    T_ASSERT_EQ_INT((int)a.len, 3);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_DEC);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_MINUS);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_question_and_colon_tokens(void) {
    /* "a ? b : c" → IDENT QUESTION IDENT COLON IDENT EOF */
    TokenArray a = lex_str("a ? b : c");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_QUESTION);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_COLON);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

static void test_question_colon_no_spaces(void) {
    /* "a?b:c" → IDENT QUESTION IDENT COLON IDENT EOF */
    TokenArray a = lex_str("a?b:c");
    T_ASSERT_EQ_INT((int)a.len, 6);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[1].kind, (int)TK_QUESTION);
    T_ASSERT_EQ_INT((int)a.data[2].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[3].kind, (int)TK_COLON);
    T_ASSERT_EQ_INT((int)a.data[4].kind, (int)TK_IDENT);
    T_ASSERT_EQ_INT((int)a.data[5].kind, (int)TK_EOF);
    token_array_free(&a);
}

/* ---- main ---- */

int main(void) {
    test_keyword_package();
    test_keyword_int();
    test_keyword_return();
    test_int_literal();
    test_full_program();
    test_empty_input();
    test_line_comment();
    test_block_comment();
    test_position_tracking();
    test_string_literal();
    test_char_literal_simple();
    test_char_literal_escape_n();
    test_char_literal_escape_backslash();
    test_char_literal_in_expr();
    test_keyword_import();
    test_unknown_char_dies();
    test_preprocessor_rejected();
    test_arith_op_tokens();
    test_paren_expr_tokens();
    test_comment_with_arith();
    test_slash_not_comment();
    test_assign_token();
    test_double_assign_is_two_tokens();
    test_compare_op_tokens();
    test_control_flow_keywords();
    test_logical_op_tokens();
    test_andand_not_confused_with_amp();
    test_not_token();
    test_not_not_confused_with_ne();
    test_question_and_colon_tokens();
    test_question_colon_no_spaces();
    test_bitwise_op_tokens();
    test_tilde_not_confused();
    test_shl_shr_tokens();
    test_shl_not_confused_with_lt();
    test_bitor_not_confused_with_oror();
    test_inc_dec_tokens();
    test_dec_not_confused_with_arrow();
    test_compound_assign_tokens();
    test_shift_compound_tokens();
    test_compound_not_confused();
    return t_finalize();
}
