// expect: 131
// Global array in the .data segment.
package main;
char buf[16];
int main() {
    buf[0] = 65;
    buf[1] = 66;
    return buf[0] + buf[1];
}
