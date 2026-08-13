// expect: 42
// Nested switch statements.
package main;
int main(void) {
    int a = 1;
    int b = 2;
    int r = 0;
    switch (a) {
        case 1:
            switch (b) {
                case 2: r = 42; break;
                default: r = 1; break;
            }
            break;
        default:
            r = 2;
            break;
    }
    return r;
}
