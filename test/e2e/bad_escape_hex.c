// expect_error
// A hex escape with no digits (`\x` not followed by a hex digit) is a lex
// error.  fakecc must reject this at compile time.
package main;
int main() {
    char c = '\x';
    return c;
}
