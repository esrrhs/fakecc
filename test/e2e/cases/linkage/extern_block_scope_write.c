// expect: 7
// A write through a block-scope `extern` must land in the global, so a later
// read from a different function observes it.
package main;
int g_val = 0;
void bump(void) {
    extern int g_val;
    g_val = g_val + 7;
}
int main() {
    bump();
    return g_val;
}
