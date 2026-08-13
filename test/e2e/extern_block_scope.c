// expect: 42
// Block-scope `extern` re-declares a file-scope variable; it must not
// allocate storage.  Regression: the declaration used to get a fresh
// zero-initialized stack slot, so `get_it` read 0 instead of the global
// that `set_it` had written.
package main;
int g_counter = 0;
void set_it(void) { g_counter = 42; }
int get_it(void) {
    extern int g_counter;
    return g_counter;
}
int main() {
    set_it();
    return get_it();
}
