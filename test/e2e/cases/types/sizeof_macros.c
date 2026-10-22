// GCC predefined __SIZEOF_* macros + __SIZE_TYPE__ / __WCHAR_TYPE__ /
// __WINT_TYPE__ / __PTRDIFF_TYPE__ typedefs.  Each macro should equal the
// real sizeof of the corresponding C type; the typedefs must be usable as
// type names.  Mirrors test/compile/sizeof-macros-1.c but actually runs.
// expect: 0
package main;

int main() {
    /* __SIZEOF_* vs sizeof(real type) */
    if (__SIZEOF_INT__ != sizeof(int)) return 1;
    if (__SIZEOF_LONG__ != sizeof(long)) return 2;
    if (__SIZEOF_LONG_LONG__ != sizeof(long long)) return 3;
    if (__SIZEOF_SHORT__ != sizeof(short)) return 4;
    if (__SIZEOF_POINTER__ != sizeof(void*)) return 5;
    if (__SIZEOF_FLOAT__ != sizeof(float)) return 6;
    if (__SIZEOF_DOUBLE__ != sizeof(double)) return 7;
    if (__SIZEOF_LONG_DOUBLE__ != sizeof(long double)) return 8;

    /* __SIZE_TYPE__ and friends are real typedefs the lexer macro fold
     * must not shadow.  Compare them both ways. */
    if (sizeof(__SIZE_TYPE__) != __SIZEOF_SIZE_T__) return 9;
    if (__SIZEOF_SIZE_T__ != sizeof(unsigned long)) return 10;

    if (sizeof(__PTRDIFF_TYPE__) != __SIZEOF_PTRDIFF_T__) return 11;
    if (__SIZEOF_PTRDIFF_T__ != sizeof(long)) return 12;

    if (sizeof(__WCHAR_TYPE__) != __SIZEOF_WCHAR_T__) return 13;
    if (sizeof(__WINT_TYPE__) != __SIZEOF_WINT_T__) return 14;

    /* Everything must agree with the compiler's own constants. */
    if (__SIZEOF_SIZE_T__ != 8) return 15;
    if (__SIZEOF_PTRDIFF_T__ != 8) return 16;
    if (__SIZEOF_WCHAR_T__ != 4) return 17;
    if (__SIZEOF_WINT_T__ != 4) return 18;
    if (__SIZEOF_SHORT__ != 2) return 19;

    return 0;
}