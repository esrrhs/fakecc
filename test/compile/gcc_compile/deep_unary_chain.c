/* Regression: a long but legitimate unary chain must be accepted.
 * Prefix operators are parsed, type-checked, and lowered iteratively, so
 * this stays inside a 1MB stack (Windows, including ASan) instead of
 * pushing one frame per operator. 200 is well under the 512-operator cap.
 */
package main;

int test_deep_unary(int x) {
    /* 200 unary ! operators - should compile without stack overflow */
    return !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!! x;
}

int main(void) {
    return test_deep_unary(1) ? 0 : 1;
}
