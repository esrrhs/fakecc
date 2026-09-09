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

int foo(int a)
{
  int b = a == 0;
  return (a & b);
}

 _Bool func_0_( int a) { int b = a == 0; return (a & b); } 
 _Bool func_0_volatile(volatile int a) { volatile int b = a == 0; return (a & b); } 
 _Bool func_1_( int a) { int b = a == 1; return (a & b); } 
 _Bool func_1_volatile(volatile int a) { volatile int b = a == 1; return (a & b); } 
 _Bool func_5_( int a) { int b = a == 5; return (a & b); } 
 _Bool func_5_volatile(volatile int a) { volatile int b = a == 5; return (a & b); }
int main(void)
{
  for(int a = -10; a <= 10; a++)
   {
     do { if(func_0_(a) != func_0_volatile(a)) abort(); } 
while(0); do { if(func_1_(a) != func_1_volatile(a)) abort(); } 
while(0); do { if(func_5_(a) != func_5_volatile(a)) abort(); } 
while(0);
   }
}

