// expect: 198
// Struct with an array member.
package main;
struct Buf { char data[16]; int len; };
int main() {
    struct Buf b;
    b.data[0] = 65;
    b.data[1] = 66;
    b.data[2] = 67;
    b.len = 3;
    int s = 0;
    for (int i = 0; i < b.len; i = i + 1) { s = s + b.data[i]; }
    return s;
}
