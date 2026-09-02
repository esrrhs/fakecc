// expect: 1
package main;

enum Tiny : bool { X };

int main(void) {
    return sizeof(enum Tiny);
}
