// expect: 1
// _Bool: normalize any non-zero scalar to 1, zero to 0.  Assignment and cast
// both normalize; reading back yields exactly 0 or 1.
package main;
int main() {
    _Bool b;
    b = 5;              /* non-zero -> 1 */
    if (b != 1) return 1;
    b = 0;
    if (b != 0) return 2;
    /* cast from int */
    if ((_Bool)42 != 1) return 3;
    if ((_Bool)0 != 0) return 4;
    /* cast from a value that truncates to 0 in the low byte but is non-zero */
    if ((_Bool)256 != 1) return 5;
    /* cast from float */
    if ((_Bool)3.5 != 1) return 6;
    if ((_Bool)0.0 != 0) return 7;
    /* comparison yields 0/1 */
    if ((5 == 5) != 1) return 8;
    return b + 1;       /* b is 0 -> returns 1 */
}
