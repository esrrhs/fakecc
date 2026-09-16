// expect: 0
// C99 6.5.16.2: `E1 op= E2` is `E1 = E1 op E2` (E1 once), so usual
// arithmetic conversions apply.  `float f; f -= 1e20;` is a double sub.
package main;

int main(void) {
    float f = 1e20f;
    f -= 1e20;
    /* Converting 1e20 to float first would cancel to 0.  UAC keeps the
     * rounding error of 1e20f vs the double literal. */
    if (f == 0.0f) return 1;

    float g = 1.0f;
    g += 1;
    if (g != 2.0f) return 3;

    double d = 1.0;
    d += 1.0f;
    if (d != 2.0) return 4;
    return 0;
}
