// skip_oracle: windows
// expect: 0
// copysignl copies the 80-bit sign bit; a double round-trip would drop it
// skip_oracle: clang
// for values that are finite as long double but overflow double.
package main;
import runtime;
int main(void) {
    long double mag = 1.0L;
    long double neg = runtime.copysignl(mag, -2.0L);
    if (!(neg < 0.0L)) return 1;
    long double pos = runtime.copysignl(-4.0L, 1.0L);
    if (!(pos > 0.0L)) return 2;
    long double huge = 1.0e4930L;
    long double nhuge = runtime.copysignl(huge, -1.0L);
    if (!(nhuge < 0.0L)) return 3;
    /* -0.0 has no sign any comparison can see, so building the magnitude by
     * hand cannot copy it either: 1.0L/-0.0L is -inf, 1.0L/+0.0L is +inf. */
    long double nz = runtime.copysignl(0.0L, -2.0L);
    if (!(1.0L / nz < 0.0L)) return 4;
    long double pz = runtime.copysignl(-0.0L, 2.0L);
    if (!(1.0L / pz > 0.0L)) return 5;
    return 0;
}
