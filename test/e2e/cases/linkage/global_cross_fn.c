// expect: 1
// Global int updated in one function, observed in another.
package main;
int state = 0;
int set_state(int v) { state = v; return 0; }
int main() {
    set_state(1);
    return state;
}
