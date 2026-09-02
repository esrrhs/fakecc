// expect: 1
// gcc_flags: -std=gnu23
package main;

enum Flag : bool { OFF = 0, ON = 1 };

int main(void) {
    enum Flag f = ON;
    if (f != ON) return 2;
    if (f != 1) return 3;
    return f;
}
