// expect: 2
package main;

enum Small : unsigned short { A = 1, B = 2 };

int main(void) {
    enum Small s = B;
    return s;
}
