// expect: 1
// gcc_flags: -std=gnu23
package main;

enum Bit : bool { ZERO, ONE };

int main(void) {
    enum Bit b = (enum Bit)1;
    return b;
}
