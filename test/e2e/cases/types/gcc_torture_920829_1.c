// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920829-1.c
package main;

long long c=2863311530LL,c3=2863311530LL*3;
int main(void){if(c*3!=c3)return 1;return 0;}