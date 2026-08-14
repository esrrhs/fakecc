// expect: 0
// Long-double IR_CONST path: each literal becomes a 10-byte rodata global
// materialized via `lea rcx,[rip+0]` + PC32 reloc + fldt [rcx] + fstpt.
// Ten live constants exercise the path under register/SSA pressure; each is
// then used in an individual comparison so none fold away.  The crash this
// guards against hit emit_module_add_undefined with a NULL call_name on this
// exact path — a NULL deref that only manifested under memory pressure.
package main;
int main(void) {
    long double a = 1.0L;
    long double b = 2.0L;
    long double c = 3.0L;
    long double d = 4.0L;
    long double e = 5.0L;
    long double f = 6.0L;
    long double g = 7.0L;
    long double h = 8.0L;
    long double i = 9.0L;
    long double j = 10.0L;
    long double sum = a + b + c + d + e + f + g + h + i + j;
    if (sum < 54.0L || sum > 56.0L) return 1;
    if (!(a == 1.0L)) return 2;
    if (!(j == 10.0L)) return 3;
    if (!(d * 2.0L == 8.0L)) return 4;
    if (!(g - f == 1.0L)) return 5;
    return 0;
}
