// expect: 0
// _Bool: normalize any scalar to 0/1 on assignment, cast, return, and global
// init.  Covers locals, globals, function params/return, and casts from int and
// float.  _Bool is width-1 unsigned; reading it back must yield exactly 0 or 1.
package main;
_Bool g = 99;  // -> 1
_Bool foo(_Bool b) { return b; }
int main() {
    _Bool a = 5;        // -> 1
    _Bool b = 0;        // -> 0
    if (a != 1) return 1;
    if (b != 0) return 2;
    if (g != 1) return 3;
    // cast from int
    if ((_Bool)42 != 1) return 4;
    if ((_Bool)0 != 0) return 5;
    // cast from a value that truncates to 0 in the low byte but is non-zero
    if ((_Bool)256 != 1) return 6;
    // cast from float
    if ((_Bool)3.5 != 1) return 7;
    if ((_Bool)0.0 != 0) return 8;
    // function param + return
    _Bool c = foo(7);   // -> 1
    _Bool d = foo(0);   // -> 0
    if (c != 1) return 9;
    if (d != 0) return 10;
    // narrowing to int
    int e = foo(5);     // -> 1
    if (e != 1) return 11;
    return 0;
}
