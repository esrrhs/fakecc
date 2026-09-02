// expect: 255
package main;

enum Byte : unsigned char { MAX = 255 };

int main(void) {
    enum Byte b = MAX;
    return b;
}
