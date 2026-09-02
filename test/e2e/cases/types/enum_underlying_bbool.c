// expect: 1
package main;

enum Flag : _Bool { OFF = 0, ON = 1 };

int main(void) {
    enum Flag f = ON;
    return f;
}
