// expect: 0
// scanf %[] scansets, including GNU a-z ranges and a leading ].
package main;
import runtime;
int main(void) {
    char a[32];
    char b[32];
    int r;

    r = runtime.sscanf("hello world", "%[a-z] %s", a, b);
    if (r != 2) return 1;
    if (runtime.strcmp(a, "hello") != 0) return 2;
    if (runtime.strcmp(b, "world") != 0) return 3;

    r = runtime.sscanf("abc123", "%[^0-9]%s", a, b);
    if (r != 2) return 4;
    if (runtime.strcmp(a, "abc") != 0) return 5;
    if (runtime.strcmp(b, "123") != 0) return 6;

    r = runtime.sscanf("]xyz", "%[]a]%s", a, b);
    if (r != 2) return 7;
    if (runtime.strcmp(a, "]") != 0) return 8;
    if (runtime.strcmp(b, "xyz") != 0) return 9;

    r = runtime.sscanf("hello", "%[a-z]", a);
    if (r != 1) return 10;
    if (runtime.strcmp(a, "hello") != 0) return 11;

    r = runtime.sscanf("123", "%[a-z]", a);
    if (r != 0) return 12;

    /* glibc %p accepts a leading sign, like %x. */
    {
        void *p = (void *)0;
        r = runtime.sscanf("-10", "%p", &p);
        if (r != 1) return 13;
        if (p != (void *)(unsigned long)0xfffffffffffffff0UL) return 14;
        p = (void *)0;
        r = runtime.sscanf("+0xabc", "%p", &p);
        if (r != 1) return 15;
        if (p != (void *)(unsigned long)0xabc) return 16;
        p = (void *)0;
        r = runtime.sscanf("-0x10", "%p", &p);
        if (r != 1) return 17;
        if (p != (void *)(unsigned long)0xfffffffffffffff0UL) return 18;
    }
    return 0;
}
