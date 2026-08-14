// NaN compares false against everything except !=, including against itself.
// ucomisd reports unordered as ZF=PF=CF=1, which reads as "equal" unless the
// parity flag is folded in and the ordered relations are taken as b>a / b>=a.
// expect: 42
// expect_stdout: 000001 000001 000001
package main;


import runtime;
int main() {
    double huge = 1e308;
    double inf = huge * 10.0;
    double nan = inf - inf;
    double one = 1.0;
    runtime.printf("%d%d%d%d%d%d %d%d%d%d%d%d %d%d%d%d%d%d\n",
           nan < one, nan <= one, nan > one, nan >= one, nan == one, nan != one,
           one < nan, one <= nan, one > nan, one >= nan, one == nan, one != nan,
           nan < nan, nan <= nan, nan > nan, nan >= nan, nan == nan, nan != nan);
    if (nan == nan) return 1;
    if (!(nan != nan)) return 2;
    if (nan < one || nan > one || nan <= one || nan >= one) return 3;
    float fnan = (float)nan;
    if (fnan == fnan || fnan < 1.0f) return 4;
    long double lnan = (long double)nan;
    if (lnan == lnan || lnan < 1.0L) return 5;
    return 42;
}
