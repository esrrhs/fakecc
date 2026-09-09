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


int t1(float *f, int i, void (*f1)(double), void (*f2)(float, float)) {
    f1(3.0);
    f[i] = f[i + 1];
    f2(2.5f, 3.5f);
    return 0;
}
int t2(float *f, int i, void (*f1)(double), void (*f2)(float, float), void (*f3)(float)) {
    f3(6.0f);
    f1(3.0);
    f[i] = f[i + 1];
    f2(2.5f, 3.5f);
    return 0;
}
void f1(double d) { if (d != 3.0) abort(); }
void f2(float a, float b) { if (a != 2.5f || b != 3.5f) abort(); }
void f3(float f) { if (f != 6.0f) abort(); }
int main(void) {
    float f[3];
    f[0] = 2.0f; f[1] = 3.0f; f[2] = 4.0f;
    t1(f, 0, f1, f2);
    t2(f, 1, f1, f2, f3);
    if (f[0] != 3.0f && f[1] != 4.0f) abort();
    return 0;
}
