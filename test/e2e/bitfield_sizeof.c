// expect: 4
// 3+5+4 bits pack into one 4-byte unsigned unit.
package main;
struct F {
    unsigned a : 3;
    unsigned b : 5;
    unsigned c : 4;
};
int main(void) {
    return (int)sizeof(struct F);
}
