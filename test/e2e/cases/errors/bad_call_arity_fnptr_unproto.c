// expect_error
// `int (*fp)()` is unprototyped in GNU C, but FakeCC still requires zero args.
package main;
int add(int x, int y) { return x + y; }
int main(void) {
    int (*fp)();
    fp = add;
    return fp(2, 3);
}
