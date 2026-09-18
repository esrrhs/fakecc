// expect: 0
// MEMORY-class aggregates larger than 128 bytes are still passed by value
// (copied on the stack), not decayed to a pointer.
package main;
struct Big { char a[200]; int tag; };
int sum(struct Big b) {
    return (int)(unsigned char)b.a[0] + (int)(unsigned char)b.a[199] + b.tag;
}
int main(void) {
    struct Big b;
    int i = 0;
    while (i < 200) {
        b.a[i] = 0;
        i = i + 1;
    }
    b.a[0] = 3;
    b.a[199] = 5;
    b.tag = 7;
    if (sum(b) != 15) return 1;
    if (b.tag != 7) return 2;
    return 0;
}
