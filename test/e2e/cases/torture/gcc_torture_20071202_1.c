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


struct T { int t; int r[8]; };
struct S { int a; int b; int c[6]; struct T d; };
void foo(struct S *s) {
    int a = s->b;
    int b = s->a;
    struct T d = s->d;
    s->a = a;
    s->b = b;
    s->c[0]=s->c[1]=s->c[2]=s->c[3]=s->c[4]=s->c[5]=0;
    s->d = d;
}
int main(void) {
    struct S s;
    int i;
    s.a = 6; s.b = 12;
    for (i = 0; i < 6; i++) s.c[i] = i+1;
    s.d.t = 7;
    for (i = 0; i < 8; i++) s.d.r[i] = i+8;
    foo(&s);
    if (s.a != 12 || s.b != 6) abort();
    for (i = 0; i < 6; i++) if (s.c[i]) abort();
    if (s.d.t != 7) abort();
    for (i = 0; i < 8; i++) if (s.d.r[i] != i+8) abort();
    return 0;
}
