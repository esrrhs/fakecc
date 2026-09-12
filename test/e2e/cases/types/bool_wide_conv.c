// expect: 0
// _Bool conversion must test the full source width.  Truncating to 32 bits
// before `!= 0` would turn `1LL<<32` and a pointer with a clear low half into 0.
package main;
int main(void) {
    _Bool a = (_Bool)(1LL << 32);
    if (a != 1) return 1;
    if ((_Bool)(1LL << 32) != 1) return 2;
    if ((_Bool)0LL != 0) return 3;
    void *p = (void *)0x100000000UL;
    if ((_Bool)p != 1) return 4;
    if ((_Bool)(void *)0 != 0) return 5;
    unsigned long long u = 0x100000000ULL;
    _Bool b = u;
    if (b != 1) return 6;
    return 0;
}
