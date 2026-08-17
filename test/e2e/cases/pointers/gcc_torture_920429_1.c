// expect: 0
// Ported from GCC C-Torture: gcc.c-torture/execute/920429-1.c
package main;

/* { dg-additional-options "-std=gnu89" } */
typedef unsigned char t;int i,j;
t*f(t*p){t c;c=*p++;i=((c&2)?1:0);j=(c&7)+1;return p;}
int main(){t*p0="ab",*p1;p1=f(p0);if(p0+1!=p1)return 1;return 0;}