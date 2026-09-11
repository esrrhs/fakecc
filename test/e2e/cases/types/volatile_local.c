// expect: 2
// Volatile locals must not be promoted out of memory: stores and the
// subsequent load of `x` are observable.  Address-taken poke catches a
// mem2reg that would ignore an aliased write.
package main;
void poke(volatile int *p) { *p = 99; }
int main(void) {
    volatile int x = 0;
    x = 1;
    x = 2;
    if (x != 2) return 1;
    volatile int y = 1;
    poke(&y);
    if (y != 99) return 3;
    return 2;
}
