/* Regression test for stack overflow in deeply nested unary operators.
 * The parser and semantic analyzer must limit expression nesting depth
 * to avoid stack overflow under ASan/UBSan or in resource-constrained
 * environments.
 *
 * This test uses a chain of 200 unary NOT operators, which is well below
 * the MAX_EXPR_DEPTH limit (512) but deep enough to have triggered stack
 * overflow under ASan before the fix.
 */
package main;

int test_deep_unary(int x) {
    /* 200 unary ! operators - should compile without stack overflow */
    return !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! x;
}

int main(void) {
    return test_deep_unary(1) ? 0 : 1;
}
