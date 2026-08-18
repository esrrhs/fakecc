// expect: 0
package main;

extern void* memcpy(void*, const void*, unsigned long);
extern void* memset(void*, int, unsigned long);
extern int memcmp(const void*, const void*, unsigned long);
extern void* memmove(void*, const void*, unsigned long);
extern int strcmp(const char*, const char*);
extern int strncmp(const char*, const char*, unsigned long);
extern unsigned long strlen(const char*);
extern char* strcpy(char*, const char*);
extern char* strncpy(char*, const char*, unsigned long);
extern char* strchr(const char*, int);
extern char* strrchr(const char*, int);
extern char* strcat(char*, const char*);
extern char* strncat(char*, const char*, unsigned long);
extern char* strstr(const char*, const char*);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int puts(const char*);
extern int putchar(int);
extern void* malloc(unsigned long);
extern void free(void*);
extern void* calloc(unsigned long, unsigned long);
extern void* realloc(void*, unsigned long);
extern int abs(int);
extern long labs(long);
extern int atoi(const char*);
extern long atol(const char*);
extern double atof(const char*);
extern double sqrt(double);
extern double fabs(double);
extern double pow(double, double);
extern double ceil(double);
extern double floor(double);
extern void exit(int);
extern void abort(void);
extern int rand(void);
extern void srand(unsigned int);

extern void abort (void);
extern void exit (int);
int
main(void)
{
    int a,b,c;
    int *pa, *pb, *pc;
    int **ppa, **ppb, **ppc;
    int i,j,k,x,y,z;
    a = 10;
    b = 20;
    c = 30;
    pa = &a; pb = &b; pc = &c;
    ppa = &pa; ppb = &pb; ppc = &pc;
    x = 0; y = 0; z = 0;
    for(i=0;i<10;i++){
        if( pa == &a ) pa = &b;
        else pa = &a;
        while( (*pa)-- ){
            x++;
            if( (*pa) < 3 ) break;
            else pa = &b;
        }
        x++;
        pa = &b;
    }
    if ((*pa) != -5 || (*pb) != -5 || x != 43)
      abort ();
    exit (0);
}
