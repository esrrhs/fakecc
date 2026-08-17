// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920711-1.c
package main;

/* { dg-options "-fwrapv" } */

int f(long a){return (--a > 0);}
int main(){if(f(0x80000000L)==0)return 1;return 0;}