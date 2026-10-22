// expect: 9
/* Typedefs in different functions are independent scopes. */
package main;
int f(void) {
    typedef long U;
    return (int)sizeof(U);
}
int g(void) {
    typedef char U;
    return (int)sizeof(U);
}
int main() {
    return f() + g();
}
