// expect: 1
package main;

enum Byte : unsigned char { A };

int main(void) {
    return sizeof(enum Byte);
}
