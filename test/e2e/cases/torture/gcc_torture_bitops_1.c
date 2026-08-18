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
extern void* memchr(const void*, int, unsigned long);
extern int printf(const char*, ...);
extern int sprintf(char*, const char*, ...);
extern int snprintf(char*, unsigned long, const char*, ...);
extern int fprintf(void*, const char*, ...);
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


int h0(int A, int B) { return (A | B) & (A == B); }
int i0(int A, int B) { return A | (A == B); }
int k0(int A, int B) { return (A & B) | (A == B); }
int h1(volatile int A, volatile int B) { return (A | B) & (A == B); }
int i1(volatile int A, volatile int B) { return A | (A == B); }
int k1(volatile int A, volatile int B) { return (A & B) | (A == B); }
int values[8];
int numvalues = 8;
int main(void) {
    int A, B;
    values[0]=0; values[1]=1; values[2]=2; values[3]=3;
    values[4]=-1; values[5]=-2; values[6]=-3; values[7]=0x10080;
    for (A = 0; A < numvalues; A++)
        for (B = 0; B < numvalues; B++) {
            int a = values[A];
            int b = values[B];
            if (h0(a, b) != h1(a, b)) abort();
            if (i0(a, b) != i1(a, b)) abort();
            if (k0(a, b) != k1(a, b)) abort();
        }
    return 0;
}
