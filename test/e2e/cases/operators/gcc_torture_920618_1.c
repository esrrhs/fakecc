// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920618-1.c
package main;

int main(void){if(1.17549435e-38F<=1.1)return 0;return 1;}