// expect: 0
// Stores into a bitfield must mask to the field width; neighbours stay intact.
package main;
struct F {
    unsigned lo : 4;
    unsigned hi : 4;
};
int main(void) {
    struct F f;
    f.lo = 0;
    f.hi = 0;
    f.lo = 0xff; /* 8 bits → keep low 4 → 15 */
    if (f.lo != 15) return 1;
    if (f.hi != 0) return 2;
    f.hi = 0xab; /* → 11 */
    if (f.hi != 11) return 3;
    if (f.lo != 15) return 4;
    return 0;
}
