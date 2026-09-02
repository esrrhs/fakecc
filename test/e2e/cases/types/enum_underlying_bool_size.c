// expect: 1
// gcc_flags: -std=gnu23
package main;

enum Tiny : bool { X };

int main(void) {
    return sizeof(enum Tiny);
}
