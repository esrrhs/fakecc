// expect: 0
// Sole-use frame loads into ADD/SUB should remat into the ALU acc
// (no stale-home use after the LOAD is fused away).
package main;

int main(void) {
    int a = 10;
    int b = 3;
    int i;
    int acc = 0;
    for (i = 0; i < 8; i = i + 1) {
        acc = acc + (a + b);
        acc = acc + (i + a);
        acc = acc - b;
        a = a + 1;
    }
    /* i=0..7: start a=10,b=3
       each iter: acc += (a+b); acc += (i+a); acc -= b; a++
       i=0 a=10: +13 +10 -3 => 20, a=11
       i=1 a=11: +14 +12 -3 => 43, a=12
       i=2 a=12: +15 +14 -3 => 69, a=13
       i=3 a=13: +16 +16 -3 => 98, a=14
       i=4 a=14: +17 +18 -3 => 130, a=15
       i=5 a=15: +18 +20 -3 => 165, a=16
       i=6 a=16: +19 +22 -3 => 203, a=17
       i=7 a=17: +20 +24 -3 => 244, a=18
    */
    if (acc != 244) return 1;
    if (a != 18) return 2;
    return 0;
}
