// expect: 0
// __INT_MIN__ / __LONG_MIN__ must parse as the closed endpoints, not as a
// leading-minus integer token (which the parser rejects) and not as MAX-1.
package main;
int main(void) {
    if (__INT_MIN__ != -2147483647 - 1) return 1;
    if (__LONG_MIN__ != -9223372036854775807l - 1l) return 2;
    switch (0) {
    case __INT_MIN__ + 2147483647 + 1:
        break;
    default:
        return 3;
    }
    return 0;
}
