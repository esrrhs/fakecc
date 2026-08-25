// expect: 0
package main;
extern void* memcpy(void*, const void*, unsigned long);
struct big { int i[0x4000]; };
struct big gb;
int foo(struct big b, int x) { return b.i[x]; }
int main(void) { return foo(gb, 0) + foo(gb, 1); }
