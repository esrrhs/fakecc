// expect: 0
package main;

extern int sprintf(char *str, const char *format, ...);
void abort (void);
void exit (int);
double normalize(x)double x;{if(x==0)x=0;return x;}
int main(void){char b[9];sprintf(b,"%g",normalize(-0.0));if(__builtin_strcmp(b,"0"))abort();exit(0);}
