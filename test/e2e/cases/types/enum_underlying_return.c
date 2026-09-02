// expect: 2
package main;

enum Code : unsigned char { A = 1, B = 2 };

enum Code pick(int n) {
    if (n) return B;
    return A;
}

int main(void) {
    return pick(1);
}
