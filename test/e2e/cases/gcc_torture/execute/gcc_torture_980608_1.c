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

extern void abort(void);
extern void exit (int);
void f1(int a,int b,int c,int d,int e, int f,int g,int h,int i,int j, int k,int
l,int m,int n,int o)
{
    return;
}
 void debug(const char *msg, ...)
{
    va_list ap;
    va_start(ap, msg);
    f1(1,2,3,4,5,6,7,8,9,10,11,12,13,14,15);
    if ( va_arg(ap, int) != 101)
        abort();
    if ( va_arg(ap, int) != 102)
        abort();
    if ( va_arg(ap, int) != 103)
        abort();
    if ( va_arg(ap, int) != 104)
        abort();
    if ( va_arg(ap, int) != 105)
        abort();
    if ( va_arg(ap, int) != 106)
        abort();
    va_end(ap);
}
int main(void)
{
  debug("%d %d %d  %d %d %d\n", 101, 102, 103, 104, 105, 106);
  exit(0);
}

