// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920506-1.c
package main;

int l[]={0,1};
int main(void){int*p=l;switch(*p++){case 0:return 0;case 1:break;case 2:break;case 3:case 4:break;}return 1;}