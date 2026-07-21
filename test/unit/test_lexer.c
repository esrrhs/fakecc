#include "fakecc/lexer.h"
#include "fakecc/token.h"
#include "test_framework.h"

#include <stdlib.h>
#include <string.h>

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

static void test_keyword_import(void) {
    TokenArray a = lex_str("import");
    T_ASSERT_EQ_INT((int)a.len, 2);
    T_ASSERT_EQ_INT((int)a.data[0].kind, (int)TK_KW_IMPORT);
    T_ASSERT_STR_EQ(a.data[0].text, "import");
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
    test_keyword_import();
    return t_finalize();
}
