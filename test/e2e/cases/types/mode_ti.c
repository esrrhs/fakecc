// expect: 16
package main;

typedef int ti_t __attribute__((mode(TI)));

int main(void) {
    return (int)sizeof(ti_t);
}
