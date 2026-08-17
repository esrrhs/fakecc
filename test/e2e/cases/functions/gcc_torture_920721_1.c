// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920721-1.c
package main;

long f(short a,short b){return (long)a/b;}
int main(void){if(f(-32768,-1)!=32768L)return 1;else return 0;}