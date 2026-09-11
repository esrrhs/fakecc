// expect: 0
// skip_difftest
// One `import flags` sees enum constants from every file in the package.
package main;
import flags;
int main(void) {
    if (flags.FLAG_A != 1) return 1;
    if (flags.FLAG_EXTRA != 32) return 2;
    return 0;
}
