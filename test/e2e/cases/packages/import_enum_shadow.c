// expect: 0
// A local of the same name as an imported package shadows `pkg.CONST`,
// so this is struct member access, not the imported enum value 1.
package main;
import flags;
struct S { int FLAG_A; };
int main(void) {
    if (flags.FLAG_A != 1) return 1;
    {
        struct S flags;
        flags.FLAG_A = 99;
        if (flags.FLAG_A != 99) return 2;
    }
    if (flags.FLAG_A != 1) return 3;
    return 0;
}
