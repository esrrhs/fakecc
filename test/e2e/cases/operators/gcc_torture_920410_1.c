// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920410-1.c
package main;

/* { dg-require-stack-size "40000 * 4 + 256" } */

int main(void){int d[40000];d[0]=0;return 0;}