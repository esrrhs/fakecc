// expect: 0
// void function with a bare `return;` — exits with garbage in eax, but we
// force a deterministic exit code via a side effect.
package main;
void noop(void) { return; }
int main() {
    noop();
    return 0;
}
