/* Cross-package enum constants: anonymous, tagged, negative, implicit next. */
package flags;

enum { FLAG_A = 1, FLAG_B = 2, FLAG_C = 4 };

enum { FLAG_NEG = -5, FLAG_NEXT };

enum Color {
    COLOR_RED = 10,
    COLOR_BLUE = 20
};

enum { TOKEN_A = 100, TOKEN_B };

int flag_one(void) { return FLAG_A; }
